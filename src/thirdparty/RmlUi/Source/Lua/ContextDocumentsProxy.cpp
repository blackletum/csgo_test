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
 
#include "ContextDocumentsProxy.h"
#include <RmlUi/Core/ElementDocument.h>

namespace Rml {
namespace Lua {
typedef ElementDocument Document;
template<> void ExtraInit<ContextDocumentsProxy>(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,ContextDocumentsProxy__index);
    lua_setfield(L,metatable_index,"__index");
    lua_pushcfunction(L,ContextDocumentsProxy__pairs);
    lua_setfield(L,metatable_index,"__pairs");
    lua_pushcfunction(L,ContextDocumentsProxy__ipairs);
    lua_setfield(L,metatable_index,"__ipairs");
}

int ContextDocumentsProxy__index(lua_State* L)
{
    /*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
    int type = lua_type(L,2);
    if(type == LUA_TNUMBER || type == LUA_TSTRING) //only valid key types
    {
        ContextDocumentsProxy* proxy = LuaType<ContextDocumentsProxy>::check(L,1);
        Document* ret = nullptr;
        if(type == LUA_TSTRING)
            ret = proxy->owner->GetDocument(luaL_checkstring(L,2));
        else
            ret = proxy->owner->GetDocument((int)luaL_checkinteger(L,2));
        LuaType<Document>::push(L,ret,false);
        return 1;
    }
    else
        return LuaType<ContextDocumentsProxy>::index(L);
    
}

//[1] is the object, [2] is the last used key, [3] is the userdata
int ContextDocumentsProxy__pairs(lua_State* L)
{
    Document* doc = nullptr;
    ContextDocumentsProxy* obj = LuaType<ContextDocumentsProxy>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    int* pindex = (int*)lua_touserdata(L,3);
    if((*pindex) == -1)
        *pindex = 0;

    int num_docs = obj->owner->GetNumDocuments();
    //because there can be missing indexes, make sure to continue until there
    //is actually a document at the index
    while((*pindex) < num_docs)
    {
        doc = obj->owner->GetDocument((*pindex)++);
        if(doc != nullptr)
            break;
    }

    //If we found a document 
    if(doc != nullptr)
    {
        lua_pushstring(L,doc->GetId().c_str());
        LuaType<Document>::push(L,doc);
    }
    else //if we were at the end and didn't find a document
    {
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 2;
}

//same as __pairs, but putting an integer key instead of a string key
int ContextDocumentsProxy__ipairs(lua_State* L)
{
    Document* doc = nullptr;
    ContextDocumentsProxy* obj = LuaType<ContextDocumentsProxy>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    int* pindex = (int*)lua_touserdata(L,3);
    if((*pindex) == -1)
        *pindex = 0;

    int num_docs = obj->owner->GetNumDocuments();
    //because there can be missing indexes, make sure to continue until there
    //is actually a document at the index
    while((*pindex) < num_docs)
    {
        doc = obj->owner->GetDocument((*pindex)++);
        if(doc != nullptr)
            break;
    }

    //we found a document
    if(doc != nullptr)
    {
        lua_pushinteger(L,(*pindex)-1);
        LuaType<Document>::push(L,doc);
    }
    else //we got to the end and didn't find another document
    {
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 2;
}


RegType<ContextDocumentsProxy> ContextDocumentsProxyMethods[] =
{
    { nullptr, nullptr },
};

luaL_Reg ContextDocumentsProxyGetters[] =
{
    { nullptr, nullptr },
};

luaL_Reg ContextDocumentsProxySetters[] =
{
    { nullptr, nullptr },
};

RMLUI_LUATYPE_DEFINE(ContextDocumentsProxy)

} // namespace Lua
} // namespace Rml
