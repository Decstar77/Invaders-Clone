#include "AttoAsset.h"

#define CHECK_PARAM_COUNT(x) if (lua_gettop(L) != x) { ATTOERROR("Invalid number of parameters for function %s", __func__); return -1; }
#define RET_IF_FAILED(expr) if (!(expr)) { ATTOERROR("Invalid parameters for function %s", __func__) return -1;}
#define GET_TABLE(idx) LuaTable table(L, idx)

#define GET_ENGINE_PARAM()          LeEngine *engine = (LeEngine*)LuaScript::GetUserData(L, 1);
#define GET_I32_PARAM(v, idx)       RET_IF_FAILED( lua_isnumber(L, idx) ) v = (i32)lua_tointeger(L, idx)
#define GET_U32_PARAM(v, idx)       RET_IF_FAILED( lua_isnumber(L, idx) ) v = (u32)lua_tointeger(L, idx)
#define GET_F32_PARAM(v, idx)       RET_IF_FAILED( lua_isnumber(L, idx) ) v = (f32)lua_tonumber(L, idx)
#define GET_BOOL_PARAM(v, idx)      RET_IF_FAILED( lua_isboolean(L, idx) ) v = lua_toboolean(L, idx)
#define GET_STRING_PARAM(v, idx)    RET_IF_FAILED( lua_isstring(L, idx) ) v = lua_tostring(L, idx)

#define GET_OPTIONAL_F32_PARAM(v, idx)       if( idx <= lua_gettop(L) && lua_isnumber(L, idx) ) { v = (f32)lua_tonumber(L, idx); }
#define GET_OPTIONAL_BOOL_PARAM(v, idx)      if( idx <= lua_gettop(L) && lua_isboolean(L, idx) ) { v = lua_toboolean(L, idx); }


#define PUSH_I32_PARAM(v)           lua_pushinteger(L, v)
#define PUSH_U32_PARAM(v)           lua_pushinteger(L, v)

namespace atto
{
  
}
