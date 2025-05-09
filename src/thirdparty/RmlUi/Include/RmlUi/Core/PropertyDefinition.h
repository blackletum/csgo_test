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

#ifndef RMLUI_CORE_PROPERTYDEFINITION_H
#define RMLUI_CORE_PROPERTYDEFINITION_H

#include "Header.h"
#include "Property.h"
#include "PropertyParser.h"

namespace Rml {

/**
	@author Peter Curry
 */

enum class RelativeTarget { None, ContainingBlockWidth, ContainingBlockHeight, FontSize, ParentFontSize, LineHeight };

class RMLUICORE_API PropertyDefinition final
{
public:
	PropertyDefinition(PropertyId id, const String& default_value, bool inherited, bool forces_layout);
	PropertyDefinition(const PropertyDefinition &) = delete; 
	PropertyDefinition& operator=(const PropertyDefinition &) = delete;
	~PropertyDefinition();

	/// Registers a parser to parse values for this definition.
	/// @param[in] parser_name The name of the parser (default parsers are 'string', 'keyword', 'number' and 'colour').
	/// @param[in] parser_parameters A comma-separated list of validation parameters for the parser.
	/// @return This property definition.
	PropertyDefinition& AddParser(const String& parser_name, const String& parser_parameters = "");

	/// Set target for relative units when resolving sizes such as percentages.
	PropertyDefinition& SetRelativeTarget(RelativeTarget relative_target);

	/// Called when parsing a RCSS declaration.
	/// @param property[out] The property to set the parsed value onto.
	/// @param value[in] The raw value defined for this property.
	/// @return True if all values were parsed successfully, false otherwise.
	bool ParseValue(Property& property, const String& value) const;
	/// Called to convert a parsed property back into a value.
	/// @param value[out] The string to return the value in.
	/// @param property[in] The processed property to parse.
	/// @return True if the property was reverse-engineered successfully, false otherwise.
	bool GetValue(String& value, const Property& property) const;

	/// Returns true if this property is inherited from parent to child elements.
	bool IsInherited() const;

	/// Returns true if this property forces a re-layout when changed.
	bool IsLayoutForced() const;

	/// Returns the default defined for this property.
	const Property* GetDefaultValue() const;

	/// Returns the target for resolving values with percent and possibly number units.
	RelativeTarget GetRelativeTarget() const;

	/// Return the property id
	PropertyId GetId() const;

private:
	PropertyId id;

	Property default_value;
	bool inherited;
	bool forces_layout;

	struct ParserState
	{
		PropertyParser* parser;
		ParameterMap parameters;
	};

	Vector< ParserState > parsers;

	RelativeTarget relative_target;
};

} // namespace Rml
#endif
