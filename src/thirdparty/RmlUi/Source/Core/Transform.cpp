﻿/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2014 Markus SchÃ¶ngart
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

#include "../../Include/RmlUi/Core/Transform.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include "../../Include/RmlUi/Core/Property.h"

namespace Rml {

// Default constructor, initializes an identity transform
Transform::Transform()
{
}

Transform::Transform(PrimitiveList primitives) 
	: primitives(primitives)
{
}

Property Transform::MakeProperty(PrimitiveList primitives)
{
	Property p{ TransformPtr{new Transform{primitives}}, Property::TRANSFORM };
	p.definition = StyleSheetSpecification::GetProperty(PropertyId::Transform);
	return p;
}

void Transform::ClearPrimitives() 
{
	primitives.clear();
}

void Transform::AddPrimitive(const TransformPrimitive & p)
{
	primitives.push_back(p);
}

int Transform::GetNumPrimitives() const noexcept 
{
	return (int)primitives.size();
}

const TransformPrimitive & Transform::GetPrimitive(int i) const noexcept 
{
	return primitives[i];
}

} // namespace Rml
