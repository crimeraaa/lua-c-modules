/**
 * @name    User-Defined Types in C
 * @link    https://www.lua.org/pil/28.html
 * 
 * @note    Recall that in Lua, each and every function has it's own "stack frame"
 *          or window into the primary stack. Think of it like a read-write view
 *          or slice into the VM stack.
 * 
 *          [0] is usually the function object itself.
 *          [1] is usually the first argument, if applicable.
 *          So on and so forth.
 *          
 * @note    In the Lua 5.1 manual, the right-hand-side notation [-o, +p, x] is:
 *          
 *          -o:     Number of values popped by the function.
 *          +p:     Number of values pushed by the function.
 *          x:      Type of error, if any, thrown by the function.
 * 
 *          Note that at least for Lua 5.1, function arguments are popped before
 *          the return values.
 *
 * @see     https://www.lua.org/manual/5.1/manual.html#3.7
 */
#define LUA_BUILD_AS_DLL
#define LUA_LIB
#include <string.h>
#include <lua.h>
#include <lauxlib.h>

#define LIB_NAME   "array"
#define LIB_NAME_Q LUA_QL(LIB_NAME)
#define MT_NAME    ("LuaBook." LIB_NAME)

typedef struct {
    int    length;
    double values[1]; // Pseudo-VLA for compatibility with C89.
} NumArray;

#define size_of_array(T, n)  (n) * sizeof(T)

// Adapted from: https://www.lua.org/pil/24.2.3.html
static void dump_stack(lua_State *L)
{
    static int count = 1;
    printf("---BEGIN STACK DUMP #%i\n", count);
    int top = lua_gettop(L);
    for (int i = 1; i <= top; i++) {
        int t = lua_type(L, i);
        printf("[%i] ", i);
        switch (t) {
        case LUA_TNIL:
            printf("nil");
            break;
        case LUA_TBOOLEAN:
            printf(lua_toboolean(L, i) ? "true" : "false");
            break;
        case LUA_TNUMBER:
            printf("%g", lua_tonumber(L, i));
            break;
        case LUA_TSTRING:
            printf("'%s'", lua_tostring(L, i));
            break;
        default:
            printf("%s (%p)", lua_typename(L, i), lua_topointer(L, i));
            break;
        }
        printf("\n");
    }
    printf("---END STACK DUMP #%i\n\n", count);
    count += 1;
}

// @details From: [ len ]
//          To:   [ inst ]
//
// @see     https://www.lua.org/pil/28.1.html
static int new_array(lua_State *L)
{
    NumArray *a;
    int       len = luaL_checkint(L, 1);
    size_t    vla = size_of_array(a->values[0], len);
    
    // Allocate an arbitrary block of memory that can only be manipulated within
    // C but is memory managed by Lua.
    // Subtract 1 sizeof(data) as we already have the singular array element.
    a = lua_newuserdata(L, sizeof(*a) + vla - sizeof(a->values[0]));
    luaL_getmetatable(L, MT_NAME); // [ len, a, mt ]
    lua_setmetatable(L, -2);       // [ len, a ],  setmetatable(a, mt)
    memset(&a->values[0], 0, vla);
    a->length = len;
    return 1;
}

/**
 * @brief   Assumes `a` is argument 1. May throw if metatable does not match.
 *          `luaL_checkudata()` checks the stack index against the given
 *          metatable name. This name may be found in the registry.
 *        
 * @note    As of Lua 5.1 `luaL_checkudata()` throws by itself.
 *          See: https://www.lua.org/manual/5.1/manual.html#7.3
 */
static NumArray *check_array(lua_State *L)
{
    return luaL_checkudata(L, 1, MT_NAME);
}

// Assumes `i` is argument 2. May throw if non-number.
static int check_index(lua_State *L, NumArray *a)
{
    int i = luaL_checkint(L, 2);
    luaL_argcheck(L, 1 <= i && i <= a->length, 2, "index out of range");
    return i;
}

static double *get_item(lua_State *L)
{
    NumArray *a = check_array(L);
    return &a->values[check_index(L, a) - 1];
}

// From: [ self, index ]
// To:   [ self[index] ]
static int get_array(lua_State *L)
{
    lua_pushnumber(L, *get_item(L));
    return 1;
}

// From: [ self, key ]
// To:   [ array[key] ]
static int get_field(lua_State *L)
{
    const char *s = luaL_checkstring(L, 2);
    lua_getglobal(L, LIB_NAME);
    lua_getfield(L, -1, s); // push array[s]
    return 1;
}

// From:   [ self, index, value ]
// To:     []
// Side:   self.values[index] = value
static int set_array(lua_State *L)
{
    *get_item(L) = luaL_checknumber(L, 3);
    return 0;
}

// From: [ self ]
// To:   [ self:length() ]
static int length_array(lua_State *L)
{
    lua_pushnumber(L, check_array(L)->length);
    return 1;
}

// From: [ inst ]
// To:   [ tostring(inst) ]
static int mt_tostring(lua_State *L)
{
    NumArray *a = check_array(L);
    lua_pushfstring(L, "array(%d)", a->length);
    return 1;
}

// From: [ inst, key ]
// To:   [ inst[key] ]
static int mt_index(lua_State *L)
{
    switch (lua_type(L, 2)) {
    case LUA_TNUMBER: return get_array(L);
    case LUA_TSTRING: return get_field(L);
    default:          break;
    }

    lua_getglobal(L, "tostring"); // [ inst, key, tostring ]
    lua_pushvalue(L, 2);          // [ inst, key, tostring, key ]
    lua_call(L, 1, 1);            // [ inst, key, tostring(key) ]
    const char *s = lua_tostring(L, -1);
    lua_pop(L, 1); // [ inst, key ]
    return luaL_error(L, "Bad " LIB_NAME_Q " field " LUA_QS, s);
}

static const luaL_reg lib_array[] = {
    {"new",        &new_array},
    {"get",        &get_array},
    {"set",        &set_array},
    {"length",     &length_array},
    {NULL,         NULL},
};

static const luaL_reg mt_array[] = {
    {"__index",    &mt_index},
    {"__newindex", &set_array},
    {"__tostring", &mt_tostring},
    {NULL,         NULL},
};

LUALIB_API int luaopen_array(lua_State *L)
{
    // See:
    // https://www.lua.org/manual/5.1/manual.html#luaL_newmetatable
    // https://www.lua.org/manual/5.1/manual.html#luaL_register
    // Unique metatable identifier that hopefully does not clash.
    luaL_newmetatable(L, MT_NAME);         // [ mt ]
    luaL_register(L, NULL, mt_array);      // [ mt ], reg. mt_array to mt (top)
    luaL_register(L, LIB_NAME, lib_array); // [ mt, array ], reg. lib_array to _G.array
    lua_pop(L, 2);                         // []
    return 0;
}
