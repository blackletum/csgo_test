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

#ifndef RMLUI_CORE_DATAEXPRESSION_H
#define RMLUI_CORE_DATAEXPRESSION_H

#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "../../Include/RmlUi/Core/DataTypes.h"

namespace Rml {

class Element;
class DataModel;
struct InstructionData;
using Program = Vector<InstructionData>;
using AddressList = Vector<DataAddress>;

class DataExpressionInterface {
public:
    DataExpressionInterface() = default;
    DataExpressionInterface(DataModel* data_model, Element* element, Event* event = nullptr);

    DataAddress ParseAddress(const String& address_str) const;
    Variant GetValue(const DataAddress& address) const;
    bool SetValue(const DataAddress& address, const Variant& value) const;
    bool CallTransform(const String& name, Variant& inout_result, const VariantList& arguments);
    bool EventCallback(const String& name, const VariantList& arguments);

private:
    DataModel* data_model = nullptr;
    Element* element = nullptr;
    Event* event = nullptr;
};


class DataExpression {
public:
    DataExpression(String expression);
    ~DataExpression();

    bool Parse(const DataExpressionInterface& expression_interface, bool is_assignment_expression);

    bool Run(const DataExpressionInterface& expression_interface, Variant& out_value);

    // Available after Parse()
    StringList GetVariableNameList() const;

private:
    String expression;
    
    Program program;
    AddressList addresses;
};

} // namespace Rml
#endif
