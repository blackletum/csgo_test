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
 
#ifndef RMLUI_LUA_ELEMENTS_ELEMENTFORMCONTROLSELECT_H
#define RMLUI_LUA_ELEMENTS_ELEMENTFORMCONTROLSELECT_H

#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>

namespace Rml {
namespace Lua {

//methods
int ElementFormControlSelectAdd(lua_State* L, ElementFormControlSelect* obj);
int ElementFormControlSelectRemove(lua_State* L, ElementFormControlSelect* obj);

//getters
int ElementFormControlSelectGetAttroptions(lua_State* L);
int ElementFormControlSelectGetAttrselection(lua_State* L);

//setter
int ElementFormControlSelectSetAttrselection(lua_State* L);

extern RegType<ElementFormControlSelect> ElementFormControlSelectMethods[];
extern luaL_Reg ElementFormControlSelectGetters[];
extern luaL_Reg ElementFormControlSelectSetters[];


//inherits from ElementFormControl which inherits from Element
template<> void ExtraInit<ElementFormControlSelect>(lua_State* L, int metatable_index);
RMLUI_LUATYPE_DECLARE(ElementFormControlSelect)
} // namespace Lua
} // namespace Rml

#endif
