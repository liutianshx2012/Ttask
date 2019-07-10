/*************************************************************************
	> File Name: latosl.c
	> Author:  TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 一  8/21 21:50:19 2017
 ************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <lua.h>
#include <lauxlib.h>

#include "atosl.h"


static int
latosl(lua_State *L)
{
    assert(lua_istable(L, 1));
    assert(lua_isnumber(L, 2));
    int index = -1;
    int count = (int)lua_tointeger(L, 2);
    char *addrs[count];

    lua_pushnil(L);
    /*  表放在索引 'tidx' 处 */
    while (lua_next(L, 1) != 0) {
        /* 使用 '键' （在索引 -2 处） 和 '值' （在索引 -1 处）*/
        assert(lua_type(L, -2) == LUA_TNUMBER &&
               lua_type(L, -1) == LUA_TSTRING);
        index++;
        addrs[index] = (char*)lua_tostring(L, -1);
        /* 移除 '值' ；保留 '键' 做下一次迭代 */
        lua_pop(L, 1);//stack: table key
    }
    const char *cpu_name = lua_tostring(L, 3);;
    const char *dsym_path = lua_tostring(L, 4);
    Dwarf_Addr load_address = LONG_MAX;
    char result[count][128];
    atosl_start_parser(result,cpu_name, dsym_path, load_address, addrs, count);

    char *format = malloc(128*count);
    for (int i=0; i<count; i++) {
        strncat(format, result[i], strlen(result[i]));
    }
    lua_pushstring(L, format);
    free(format);
    return 1;
}

LUAMOD_API int
luaopen_atosl_core(lua_State *L)
{
#ifdef luaL_checkversion
    luaL_checkversion(L);
#endif
    
    luaL_Reg l[] = {
        {"atosl",latosl},
        {NULL, NULL},
    };
    luaL_newlib(L, l);
    return 1;
}
