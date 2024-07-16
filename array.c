/**
 * @name    User-Defined Types in C
 * @link    https://www.lua.org/pil/28.html
 *          https://www.lua.org/pil/28.1.html
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

/**
 * @brief   Using `lua_newuserdata` we can allocate arbitrary blocks of memory
 *          and manipulate them within C, exposing only the object pointers to
 *          the user.
 * 
 *          Form: `a = array.new(length)`
 *
 * @see     https://www.lua.org/pil/28.1.html
 */
static int new_array(lua_State *L)
{
    NumArray *a;
    int       len = luaL_checkint(L, 1);
    size_t    vla = size_of_array(a->values[0], len);
    
    // Subtract 1 sizeof(data) as we already have the singular array element.
    a = lua_newuserdata(L, sizeof(*a) + vla - sizeof(a->values[0]));

    luaL_getmetatable(L, MT_NAME);
    lua_setmetatable(L, -2); // Set Top[-2]'s metatable to current top, pop
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

static int get_array(lua_State *L)
{
    lua_pushnumber(L, *get_item(L));
    return 1;
}

static int get_field(lua_State *L)
{
    const char *s = luaL_checkstring(L, 2);
    lua_getglobal(L, LIB_NAME);
    lua_getfield(L, -1, s); // push array[s]
    return 1;
}

/**
 * @brief   Recall that in Lua, each function has its own stack frame view.
 *          Index 0 is the function object itself, index 1 is the first argument,
 *          etc. etc.
 *
 *          Form: `array.set(array, index, value)`.
 */
static int set_array(lua_State *L)
{
    // Stack: [ array.set, inst, index, value ]
    *get_item(L) = luaL_checknumber(L, 3);
    return 0;
}

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
    if (lua_isnumber(L, 2))
        return get_array(L);
    else if (lua_isstring(L, 2))
        return get_field(L);

    lua_getglobal(L, "tostring"); // [ inst, key, tostring ]
    lua_pushvalue(L, 2);          // [ inst, key, tostring, key ]
    lua_call(L, 1, 1);            // [ inst, key, tostring(key) ]
    luaL_error(L, "Bad " LIB_NAME_Q " field " LUA_QS, lua_tostring(L, -1));
    return 1;
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

/**
 * @brief   Creates the global table `array` and fills it the functions
 *          from `lib_array`.
 */
LUALIB_API int luaopen_array(lua_State *L)
{
    // Unique metatable identifier that hopefully does not clash.
    luaL_newmetatable(L, MT_NAME);         // [mt]
    luaL_register(L, NULL, mt_array);      // Reg. `mt_array` to table @ top.
    luaL_register(L, LIB_NAME, lib_array); // Reg. `lib_array` to _G["array"].
    return 0;
}
