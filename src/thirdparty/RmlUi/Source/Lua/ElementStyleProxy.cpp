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
 
#include "ElementStyleProxy.h"
#include <RmlUi/Lua/LuaType.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/PropertiesIteratorView.h>

namespace Rml {
namespace Lua {
template<> void ExtraInit<ElementStyleProxy>(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,ElementStyleProxy__index);
    lua_setfield(L,metatable_index,"__index");

    lua_pushcfunction(L,ElementStyleProxy__newindex);
    lua_setfield(L,metatable_index,"__newindex");

    lua_pushcfunction(L,ElementStyleProxy__pairs);
    lua_setfield(L,metatable_index,"__pairs");

    lua_pushcfunction(L,ElementStyleProxy__ipairs);
    lua_setfield(L,metatable_index,"__ipairs");
}

int ElementStyleProxy__index(lua_State* L)
{
    /*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
    int keytype = lua_type(L,2);
    if(keytype == LUA_TSTRING) //if we are trying to access a string, then we will assume that it is a property
    {
        ElementStyleProxy* es = LuaType<ElementStyleProxy>::check(L,1);
        RMLUI_CHECK_OBJ(es);
        const Property* prop = es->owner->GetProperty(lua_tostring(L,2));
        RMLUI_CHECK_OBJ(prop)
        lua_pushstring(L,prop->ToString().c_str());
        return 1;
    }
    else //if it wasn't trying to get a string
    {
        lua_settop(L,2);
        return LuaType<ElementStyleProxy>::index(L);
    }
}

int ElementStyleProxy__newindex(lua_State* L)
{
    //[1] = obj, [2] = key, [3] = value
    ElementStyleProxy* es = LuaType<ElementStyleProxy>::check(L,1);
    RMLUI_CHECK_OBJ(es);
    int keytype = lua_type(L,2);
    int valuetype = lua_type(L,3);
    if(keytype == LUA_TSTRING )
    {
        const char* key = lua_tostring(L,2);
        if(valuetype == LUA_TSTRING)
        {
            const char* value = lua_tostring(L,3);
            lua_pushboolean(L,es->owner->SetProperty(key,value));
            return 1; 
        }
        else if (valuetype == LUA_TNIL)
        {
            es->owner->RemoveProperty(key);
            return 0;
        }
    }
    //everything else returns when it needs to, so we are safe to pass it
    //on if needed

    lua_settop(L,3);
    return LuaType<ElementStyleProxy>::newindex(L);

}

//[1] is the object, [2] is the last used key, [3] is the userdata
int ElementStyleProxy__pairs(lua_State* L)
{
    ElementStyleProxy* obj = LuaType<ElementStyleProxy>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    int* pindex = (int*)lua_touserdata(L,3);
	if ((*pindex) == -1)
		*pindex = 0;

	int i = 0;
	auto it = obj->owner->IterateLocalProperties();
	while (i < (*pindex) && !it.AtEnd())
	{
		++it;
		++i;
	}

    if(!it.AtEnd())
    {
		const String& key = it.GetName();
		const Property& property = it.GetProperty();
		String val;
        property.definition->GetValue(val, property);
        lua_pushstring(L,key.c_str());
        lua_pushstring(L,val.c_str());
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 2;
}

//only indexed by string
int ElementStyleProxy__ipairs(lua_State* L)
{
    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
}

RegType<ElementStyleProxy> ElementStyleProxyMethods[] = 
{
    { nullptr, nullptr },
};

luaL_Reg ElementStyleProxyGetters[] = 
{
    { nullptr, nullptr },
};

luaL_Reg ElementStyleProxySetters[] = 
{
    { nullptr, nullptr },
};

RMLUI_LUATYPE_DEFINE(ElementStyleProxy)
} // namespace Lua
} // namespace Rml
