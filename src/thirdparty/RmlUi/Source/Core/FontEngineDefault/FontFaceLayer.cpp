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

#include "FontFaceLayer.h"
#include "FontFaceHandleDefault.h"

namespace Rml {

FontFaceLayer::FontFaceLayer(const SharedPtr<const FontEffect>& _effect) : colour(255, 255, 255)
{
	effect = _effect;
	if (effect)
		colour = effect->GetColour();
}

FontFaceLayer::~FontFaceLayer()
{}

bool FontFaceLayer::Generate(const FontFaceHandleDefault* handle, const FontFaceLayer* clone, bool clone_glyph_origins)
{
	// Clear the old layout if it exists.
	{
		// @performance: We could be much smarter about this, e.g. such as adding new glyphs to the existing texture layout and textures.
		// Right now we re-generate the whole thing, including textures.
		texture_layout = TextureLayout{};
		character_boxes.clear();
		textures.clear();
	}

	const FontGlyphMap& glyphs = handle->GetGlyphs();

	// Generate the new layout.
	if (clone)
	{
		// Clone the geometry and textures from the clone layer.
		character_boxes = clone->character_boxes;

		// Copy the cloned layer's textures.
		for (size_t i = 0; i < clone->textures.size(); ++i)
			textures.push_back(clone->textures[i]);

		// Request the effect (if we have one) and adjust the origins as appropriate.
		if (effect && !clone_glyph_origins)
		{
			for (auto& pair : glyphs)
			{
				Character character = pair.first;
				const FontGlyph& glyph = pair.second;

				auto it = character_boxes.find(character);
				if (it == character_boxes.end())
				{
					// This can happen if the layers have been dirtied in FontHandleDefault. We will
					// probably be regenerated soon, just skip the character for now.
					continue;
				}

				TextureBox& box = it->second;

				Vector2i glyph_origin(Math::RealToInteger(box.origin.x), Math::RealToInteger(box.origin.y));
				Vector2i glyph_dimensions(Math::RealToInteger(box.dimensions.x), Math::RealToInteger(box.dimensions.y));

				if (effect->GetGlyphMetrics(glyph_origin, glyph_dimensions, glyph))
				{
					box.origin.x = (float)glyph_origin.x;
					box.origin.y = (float)glyph_origin.y;
				}
				else
					box.texture_index = -1;
			}
		}
	}
	else
	{
		// Initialise the texture layout for the glyphs.
		character_boxes.reserve(glyphs.size());
		for (auto& pair : glyphs)
		{
			Character character = pair.first;
			const FontGlyph& glyph = pair.second;

			Vector2i glyph_origin(0, 0);
			Vector2i glyph_dimensions = glyph.bitmap_dimensions;

			// Adjust glyph origin / dimensions for the font effect.
			if (effect)
			{
				if (!effect->GetGlyphMetrics(glyph_origin, glyph_dimensions, glyph))
					continue;
			}

			TextureBox box;
			box.origin = Vector2f(float(glyph_origin.x + glyph.bearing.x), float(glyph_origin.y - glyph.bearing.y));
			box.dimensions = Vector2f(float(glyph_dimensions.x), float(glyph_dimensions.y));
			
			RMLUI_ASSERT(box.dimensions.x >= 0 && box.dimensions.y >= 0);

			character_boxes[character] = box;

			// Add the character's dimensions into the texture layout engine.
			texture_layout.AddRectangle((int)character, glyph_dimensions);
		}

		constexpr int max_texture_dimensions = 1024;

		// Generate the texture layout; this will position the glyph rectangles efficiently and
		// allocate the texture data ready for writing.
		if (!texture_layout.GenerateLayout(max_texture_dimensions))
			return false;


		// Iterate over each rectangle in the layout, copying the glyph data into the rectangle as
		// appropriate and generating geometry.
		for (int i = 0; i < texture_layout.GetNumRectangles(); ++i)
		{
			TextureLayoutRectangle& rectangle = texture_layout.GetRectangle(i);
			const TextureLayoutTexture& texture = texture_layout.GetTexture(rectangle.GetTextureIndex());
			Character character = (Character)rectangle.GetId();
			RMLUI_ASSERT(character_boxes.find(character) != character_boxes.end());
			TextureBox& box = character_boxes[character];

			// Set the character's texture index.
			box.texture_index = rectangle.GetTextureIndex();

			// Generate the character's texture coordinates.
			box.texcoords[0].x = float(rectangle.GetPosition().x) / float(texture.GetDimensions().x);
			box.texcoords[0].y = float(rectangle.GetPosition().y) / float(texture.GetDimensions().y);
			box.texcoords[1].x = float(rectangle.GetPosition().x + rectangle.GetDimensions().x) / float(texture.GetDimensions().x);
			box.texcoords[1].y = float(rectangle.GetPosition().y + rectangle.GetDimensions().y) / float(texture.GetDimensions().y);
		}

		const FontEffect* effect_ptr = effect.get();
		const int handle_version = handle->GetVersion();

		// Generate the textures.
		for (int i = 0; i < texture_layout.GetNumTextures(); ++i)
		{
			int texture_id = i;

			TextureCallback texture_callback = [handle, effect_ptr, texture_id, handle_version](const String& /*name*/, UniquePtr<const byte[]>& data, Vector2i& dimensions) -> bool {
				bool result = handle->GenerateLayerTexture(data, dimensions, effect_ptr, texture_id, handle_version);
				return result;
			};

			Texture texture;
			texture.Set("font-face-layer", texture_callback);
			textures.push_back(texture);
		}
	}

	return true;
}

// Generates the texture data for a layer (for the texture database).
bool FontFaceLayer::GenerateTexture(UniquePtr<const byte[]>& texture_data, Vector2i& texture_dimensions, int texture_id, const FontGlyphMap& glyphs)
{
	if (texture_id < 0 ||
		texture_id > texture_layout.GetNumTextures())
		return false;

	// Generate the texture data.
	texture_data = texture_layout.GetTexture(texture_id).AllocateTexture();
	texture_dimensions = texture_layout.GetTexture(texture_id).GetDimensions();

	for (int i = 0; i < texture_layout.GetNumRectangles(); ++i)
	{
		TextureLayoutRectangle& rectangle = texture_layout.GetRectangle(i);
		Character character = (Character)rectangle.GetId();
		RMLUI_ASSERT(character_boxes.find(character) != character_boxes.end());

		TextureBox& box = character_boxes[character];

		if (box.texture_index != texture_id)
			continue;

		auto it = glyphs.find((Character)rectangle.GetId());
		if (it == glyphs.end())
			continue;

		const FontGlyph& glyph = it->second;

		if (effect == nullptr)
		{
			// Copy the glyph's bitmap data into its allocated texture.
			if (glyph.bitmap_data)
			{
				byte* destination = rectangle.GetTextureData();
				const byte* source = glyph.bitmap_data;

				for (int j = 0; j < glyph.bitmap_dimensions.y; ++j)
				{
					for (int k = 0; k < glyph.bitmap_dimensions.x; ++k)
						destination[k * 4 + 3] = source[k];

					destination += rectangle.GetTextureStride();
					source += glyph.bitmap_dimensions.x;
				}
			}
		}
		else
		{
			effect->GenerateGlyphTexture(rectangle.GetTextureData(), Vector2i(Math::RealToInteger(box.dimensions.x), Math::RealToInteger(box.dimensions.y)), rectangle.GetTextureStride(), glyph);
		}
	}

	return true;
}

// Returns the effect used to generate the layer.
const FontEffect* FontFaceLayer::GetFontEffect() const
{
	return effect.get();
}

// Returns on the layer's textures.
const Texture* FontFaceLayer::GetTexture(int index)
{
	RMLUI_ASSERT(index >= 0);
	RMLUI_ASSERT(index < GetNumTextures());

	return &(textures[index]);
}

// Returns the number of textures employed by this layer.
int FontFaceLayer::GetNumTextures() const
{
	return (int)textures.size();
}

// Returns the layer's colour.
const Colourb& FontFaceLayer::GetColour() const
{
	return colour;
}

} // namespace Rml
