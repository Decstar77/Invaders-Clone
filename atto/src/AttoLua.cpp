#include "AttoLua.h"

namespace atto 
{
    bool LuaScript::Load(const char* filename) {
        L = luaL_newstate();
        luaL_openlibs(L);

        if (luaL_dofile(L, filename) != LUA_OK) {
            ATTOERROR("Failed to load script %s", filename);
            const char *error = lua_tostring(L, -1);
            ATTOERROR("Error: %s", error);
            lua_close(L);
            L = nullptr;
            return false;
        }

        ATTOTRACE("Loaded lua script %s", filename);
        
        return true;
    }

    bool LuaScript::LoadSafe(const char* filename) {
        L = luaL_newstate();
        luaL_openlibs(L);

        if (luaL_dofile(L, filename) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            lua_close(L);
            L = nullptr;
            return false;
        }

        return true;
    }

    void LuaScript::Free() {
        if (L != nullptr) {
            lua_close(L);
            L = nullptr;
        }
    }

    bool LuaScript::GetGlobalSafe(const char* name, i32& value) {
        return GetGlobalNumberSafe(name, value);
    }

    bool LuaScript::GetGlobalSafe(const char* name, f32& value) {
        return GetGlobalNumberSafe(name, value);
    }

    bool LuaScript::GetGlobalSafe(const char* name, bool& value) {
        Assert(L != nullptr, "LuaScript::GetGlobal -> Lua state is null");

        if (lua_getglobal(L, name) != LUA_TBOOLEAN) {
            ATTOTRACE("LuaScript::GetGlobal -> Could not get global %s", name);
            return false;
        }

        if (!lua_isboolean(L, -1)) {
            ATTOTRACE("LuaScript::GetGlobal -> Could not get global %s", name);
            return false;
        }

        value = lua_toboolean(L, -1);
        
        lua_pop(L, 1);

        return true;
    }

    bool LuaScript::GetGlobal(const char* name, i32& value) {
        return GetGlobalNumber(name, value);
    }

    bool LuaScript::GetGlobal(const char* name, f32& value) {
        return GetGlobalNumber(name, value);
    }

    bool LuaScript::GetGlobal(const char* name, bool& value) {
        Assert(L != nullptr, "LuaScript::GetGlobal -> Lua state is null");
        
        if (lua_getglobal(L, name) != LUA_TBOOLEAN) {
            ATTOERROR("LuaScript::GetGlobal -> Could not get global %s", name);
            return false;
        }

        if (!lua_isboolean(L, -1)) {
            ATTOERROR("LuaScript::GetGlobal -> Global %s is not a boolean", name);
            return false;
        }
        
        value = lua_toboolean(L, -1);
        
        lua_pop(L, 1);

        return true;
    }

    void LuaScript::SetGlobal(const char* name, i32 value) {
        SetGlobalNumber(name, value);
    }

    void LuaScript::SetGlobal(const char* name, f32 value) {
        SetGlobalNumber(name, value);
    }

    void LuaScript::SetFunction(const char* name, lua_CFunction func) {
        lua_register(L, name, func);
    }

    bool LuaScript::PrepareFunction(const char* name) {
        Assert(L != nullptr, "LuaScript::GetGlobal -> Lua state is null");
        if (lua_getglobal(L, name) != LUA_TFUNCTION) {
            ATTOERROR("LuaScript::GetGlobal -> Could not get global %s", name);
            return false;
        }
        
        return true;
    }

    void LuaScript::PushValue(i32 value) {
        Assert(L != nullptr, "LuaScript::GetGlobal -> Lua state is null");
        lua_pushnumber(L, (double)value);
    }

    void LuaScript::PushValue(f32 value) {
        Assert(L != nullptr, "LuaScript::GetGlobal -> Lua state is null");
        lua_pushnumber(L, (double)value);
    }

    void LuaScript::PushValue(void* value) {
        Assert(L != nullptr, "LuaScript::GetGlobal -> Lua state is null");
        lua_pushlightuserdata(L, value);
    }

    bool LuaScript::CallFunction(i32 numArgs, i32 numResults) {
        Assert(L != nullptr, "LuaScript::GetGlobal -> Lua state is null");
        if (lua_pcall(L, numArgs, numResults, 0) != LUA_OK) {
            ATTOERROR("LuaScript::CallFunction -> Error calling function");
            return false;
        }

        return true;
    }

    void* LuaScript::GetUserData(lua_State* L, i32 index) {
        return lua_touserdata(L, index);
    }

    bool LuaScript::GetTableValue(lua_State* L, const char* key, i32 index, i32& value) {
        return GetTableValue_(L, key, index, value);
    }

    bool LuaScript::GetTableValue(lua_State* L, const char* key, i32 index, f32& value) {
        return GetTableValue_(L, key, index, value);
    }

    bool LuaScript::GetTableValue(lua_State* L, const char* key, i32 index, SmallString& value) {
        Assert(L != nullptr, "LuaScript::SetGlobal -> Lua state is null");

        if (!lua_istable(L, index)) {
            ATTOERROR("Top of stack is not table");
            return false;
        }

        lua_pushstring(L, key);
        lua_gettable(L, index);

        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            return false;
        }

        value = lua_tostring(L, -1);
        lua_pop(L, 1);

        return true;
    }

    LuaTable::LuaTable(lua_State* L, i32 index, bool isSubTable) : L(L), index(index), isSubTable(isSubTable) {
        
    }

    LuaTable::~LuaTable() {
        if (isSubTable) {
            lua_pop(L, 1);
        }
    }

    LuaTable LuaTable::operator[](const char* key) const {
        Assert(L != nullptr, "LuaTable -> Lua state is null");
        if (index == 0) {
            return LuaTable(L, 0);
        }
        
        lua_pushstring(L, key);
        lua_gettable(L, index);
        
        if (lua_isnil(L, -1)) {
            ATTOWARN("LuaTable::operator[] -> Value at key { %s } is nill", key);
            lua_pop(L, 1);
            return LuaTable(L, 0);
        }

        i32 top = lua_gettop(L);

        bool subTable = lua_istable(L, top);

        return LuaTable(L, top, subTable);
    }

    bool LuaTable::Get(i32& value) const {
        Assert(L != nullptr, "LuaTable -> Lua state is null");
        if (index == 0) {
            return false;
        }

        if (!lua_isnumber(L, index)) {
            lua_pop(L, 1);
            return false;
        }

        value = (i32)lua_tonumber(L, index);
        lua_pop(L, 1);
        
        return true;
    }

    bool LuaTable::Get(f32& value) const {
        Assert(L != nullptr, "LuaTable -> Lua state is null");
        if (index == 0) {
            return false;
        }

        if (!lua_isnumber(L, index)) {
            lua_pop(L, 1);
            return false;
        }

        value = (f32)lua_tonumber(L, index);
        lua_pop(L, 1);

        return true;
    }

    bool LuaTable::Get(SmallString& value) const {
        Assert(L != nullptr, "LuaTable -> Lua state is null");
        if (index == 0) {
            return false;
        }

        if (!lua_isstring(L, index)) {
            lua_pop(L, 1);
            return false;
        }

        value = lua_tostring(L, index);
        lua_pop(L, 1);

        return true;
    }

    bool LuaTable::Get(glm::vec2& value) const {
        Assert(L != nullptr, "LuaTable -> Lua state is null");
        if (index == 0) {
            return false;
        }

        if (!lua_istable(L, index)) {
            return false;
        }

        lua_pushnil(L);

        if (lua_next(L, index) == 0) {
            return false;
        }

        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 2); // pop value and key
            return false;
        }

        value.x = (f32)lua_tonumber(L, -1);
        lua_pop(L, 1);

        if (lua_next(L, index) == 0) {
            return false;
        }

        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 2); // pop value and key
            return false;
        }

        value.y = (f32)lua_tonumber(L, -1);
        
        lua_pop(L, 2); // 1 for the value, 1 for the ke
        
        return true;
    }

}
