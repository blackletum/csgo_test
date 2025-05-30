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

#include "InputTypeSubmit.h"
#include "../../../Include/RmlUi/Core/Elements/ElementForm.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"

namespace Rml {

InputTypeSubmit::InputTypeSubmit(ElementFormControlInput* element) : InputType(element)
{
}

InputTypeSubmit::~InputTypeSubmit()
{
}

// Submit buttons are never submitted; they submit themselves if appropriate.
bool InputTypeSubmit::IsSubmitted()
{
	return false;
}

// Checks for necessary functional changes in the control as a result of the event.
void InputTypeSubmit::ProcessDefaultAction(Event& event)
{
	if (event == EventId::Click &&
		!element->IsDisabled())
	{
		Element* parent = element->GetParentNode();
		while (parent)
		{
			ElementForm* form = rmlui_dynamic_cast< ElementForm* >(parent);
			if (form != nullptr)
			{
				form->Submit(element->GetAttribute< String >("name", ""), element->GetAttribute< String >("value", ""));
				return;
			}
			else
			{
				parent = parent->GetParentNode();
			}
		}
	}
}

// Sizes the dimensions to the element's inherent size.
bool InputTypeSubmit::GetIntrinsicDimensions(Vector2f& RMLUI_UNUSED_PARAMETER(dimensions))
{
	RMLUI_UNUSED(dimensions);
	
	return false;
}

} // namespace Rml
