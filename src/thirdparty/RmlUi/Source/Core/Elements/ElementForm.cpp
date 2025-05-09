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

#include "../../../Include/RmlUi/Core/Elements/ElementForm.h"
#include "../../../Include/RmlUi/Core/Dictionary.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControl.h"

namespace Rml {

// Constructs a new ElementForm. This should not be called directly; use the Factory instead.
ElementForm::ElementForm(const String& tag) : Element(tag)
{
}

ElementForm::~ElementForm()
{
}

// Submits the form.
void ElementForm::Submit(const String& name, const String& submit_value)
{
	Dictionary values;
	if (name.empty())
		values["submit"] = submit_value;
	else
		values[name] = submit_value;

	ElementList form_controls;
	ElementUtilities::GetElementsByTagName(form_controls, this, "input");
	ElementUtilities::GetElementsByTagName(form_controls, this, "textarea");
	ElementUtilities::GetElementsByTagName(form_controls, this, "select");
	ElementUtilities::GetElementsByTagName(form_controls, this, "dataselect");

	for (size_t i = 0; i < form_controls.size(); i++)
	{
		ElementFormControl* control = rmlui_dynamic_cast< ElementFormControl* >(form_controls[i]);
		if (!control)
			continue;

		// Skip disabled controls.
		if (control->IsDisabled())
			continue;

		// Only process controls that should be submitted.
		if (!control->IsSubmitted())
			continue;

		String control_name = control->GetName();
		String control_value = control->GetValue();

		// Skip over unnamed form controls.
		if (control_name.empty())
			continue;

		// If the item already exists, append to it.
		Variant* value = GetIf(values, control_name);
		if (value != nullptr)
			*value = value->Get< String >() + ", " + control_value;
		else
			values[control_name] = control_value;
	}

	DispatchEvent(EventId::Submit, values);
}

} // namespace Rml
