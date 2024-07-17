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

#define LIB_NAME   "dyarray"
#define LIB_NAME_Q LUA_QL(LIB_NAME)
#define MT_NAME    ("LuaBook." LIB_NAME)

#ifndef __cplusplus
#define cast(T, expr)   ((T)(expr))
#else  // __cplusplus defined.
#define cast(T, expr)   static_cast<T>(expr)
#endif // __cplusplus

typedef struct {
    int         length;   // Number of contiguous active elements.
    int         capacity; // Number of total allocated elements.
    lua_Number *values;   // Heap-allocated 1D array.
} DyArray;

#define size_of_dyarray(self, n)  ((n) * sizeof((self)->values[0]))

// Adapted from: https://www.lua.org/pil/24.2.3.html
static int dump_stack(lua_State *L)
{
    for (int i = 1, top = lua_gettop(L); i <= top; i++) {
        printf("[%i] ", i);
        switch (lua_type(L, i)) {
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
            printf("%s (%p)", luaL_typename(L, i), lua_topointer(L, i));
            break;
        }
        printf("\n");
    }
    printf("\n");
    return 0;
}

static const char *to_string(lua_State *L, int fn, int arg)
{
    const char *s;
    lua_pushvalue(L, fn);  // [ tostring]
    lua_pushvalue(L, arg); // [ tostring, arg ]
    lua_call(L, 1, 1);     // [ tostring(arg) ]
    s = lua_tostring(L, -1);
    lua_pop(L, 1); // []
    return s;
}

// From:    [ t ]
// To:      []
static int dump_table(lua_State *L)
{
    if (!lua_istable(L, 1))
        return luaL_typerror(L, 1, lua_typename(L, LUA_TTABLE));
    
    lua_getglobal(L, "tostring"); // [ t, tostring ]
    lua_pushnil(L);               // [ t, tostring, nil ] ; Set space for key.

    // https://www.lua.org/manual/5.1/manual.html#lua_next
    while (lua_next(L, 1)) {
        // Push copies so we don't confuse `lua_next()`.
        // [ t, tostring, k, t[k] ]
        printf("K: %s - V: %s\n", to_string(L, 2, 3), to_string(L, 2, 4));
        lua_pop(L, 1); // [ t, tostring, k ]
    }
    return 0;
}

static int bad_index(lua_State *L, int i)
{
    const char *tname = luaL_typename(L, -1);
    return luaL_error(L, "non-number at index %d (a %s value)", i, tname);
}

// From: [ t ]
// To:   [ self ]
//
// https://www.lua.org/pil/28.1.html
static int new_dyarray(lua_State *L)
{
    DyArray *self = lua_newuserdata(L, sizeof(*self)); // [ t, self ]
    int      len  = cast(int, lua_objlen(L, 1));
    size_t   sz   = size_of_dyarray(self, len);
    
    luaL_checktype(L, 1, LUA_TTABLE);
    
    // Allocate an arbitrary block of memory that can only be manipulated within
    // C but is memory managed by Lua.
    self->values   = lua_newuserdata(L, sz); // [ t, self, self->values ]
    self->capacity = len;
    self->length   = len;

    lua_pop(L, 1);                 // [ t, self ]
    luaL_getmetatable(L, MT_NAME); // [ t, self, mt ]
    lua_setmetatable(L, -2);       // [ t, self ] ; setmetatable(a, mt)

    for (int i = 1; i <= len; i++) {
        lua_pushinteger(L, i); // [ t, self, k ]
        lua_gettable(L, 1);    // [ t, self, t[k] ]
        if (lua_isnumber(L, -1))
            self->values[i - 1] = lua_tonumber(L, -1);
        else
            return bad_index(L, i);
        lua_pop(L, 1); // [ t, self ]
    }
    return 1;
}

/**
 * @brief   May throw if metatable does not match.
 *          `luaL_checkudata()` checks the stack index against the given
 *          metatable name. This name may be found in the registry.
 *        
 * @note    As of Lua 5.1 `luaL_checkudata()` throws by itself.
 *          See: https://www.lua.org/manual/5.1/manual.html#7.3
 */
static DyArray *check_dyarray(lua_State *L, int argn)
{
    return luaL_checkudata(L, argn, MT_NAME);
}

static int resolve_index(DyArray *self, int i)
{
    return (i < 0) ? self->length + i + 1 : i;
}

static int check_index(lua_State *L, DyArray *self, int argn)
{
    int i = resolve_index(self, luaL_checkint(L, argn));
    luaL_argcheck(L, 1 <= i && i <= self->length, argn, "index out of range");
    return i;
}

static double *get_item(lua_State *L)
{
    DyArray *self = check_dyarray(L, 1);
    return &self->values[check_index(L, self, 2) - 1];
}

// From: [ self, index ]
// To:   [ self[index] ]
static int get_dyarray(lua_State *L)
{
    lua_pushnumber(L, *get_item(L));
    return 1;
}

static int bad_field(lua_State *L, const char *s)
{
    return luaL_error(L, "Bad " LIB_NAME_Q " field " LUA_QS, s);
}

// From: [ self, key ]
// To:   [ array[key] ]
static int get_field(lua_State *L)
{
    const char *s = luaL_checkstring(L, 2);
    lua_getglobal(L, LIB_NAME); // [ self, key, mt ]
    lua_getfield(L, -1, s);     // [ self, key, mt, mt[key] ]
    return lua_isnil(L, -1) ? bad_field(L, s) : 1;
}

// From:   [ self, index, value ]
// To:     [ self ]
// Side:   self.values[index] = value
static int set_dyarray(lua_State *L)
{
    *get_item(L) = luaL_checknumber(L, 3);
    lua_pushvalue(L, 1); // [ self, index, value, self ]
    return 1;
}

// If we grow, only copy up to `len` elements.
// Otherwise, if we shrink, only copy up to `idx` elements.
static int copy_up_to(int idx, int len)
{
    return (idx >= len) ? len : idx;
}

// From:    [ self, ...args ]
// To:      [ self ]
static int resize_internal(lua_State *L, DyArray *self, int idx, int cap)
{
    // [ self, ...args, ud ]
    lua_Number *tmp = lua_newuserdata(L, size_of_dyarray(self, cap));
    memset(tmp, 0, lua_objlen(L, -1));

    // Copy old elements to new allocated memory.
    for (int i = 1, end = copy_up_to(idx, self->length); i <= end; i++)
        tmp[i - 1] = self->values[i - 1];

    self->values   = tmp;
    self->length   = idx;
    self->capacity = cap;
    lua_pop(L, 1);       // [ self, ...args ]
    lua_pushvalue(L, 1); // [ self, ...args, self ]
    return 1;
}

// From:    [ self, newsz ]
// To:      []
// Side:    resize(self, newsz)
static int resize_dyarray(lua_State *L)
{
    DyArray *self = check_dyarray(L, 1);
    int      size = luaL_checkint(L, 2);
    if (size <= 0)
        return luaL_error(L, "Cannot resize " LIB_NAME_Q " to %d items", size);
    else
        return resize_internal(L, self, size, size);
}

// From: [ self, ...args ]
// To:   [ self ]
static int insert_internal(lua_State *L, DyArray *self, int idx, lua_Number n)
{
    // Need to grow?
    if (idx > self->capacity) {
        int cap = 8;
        while (cap <= idx)
            cap *= 2;
        resize_internal(L, self, idx, cap);
    }
    self->values[idx - 1] = n;
    if (idx > self->length)
        self->length = idx;
    lua_pushvalue(L, 1);
    return 1;
}

// From:    [ self, i, v ]
// To:      [ self ]
// Side:    i > self.length ? resize(self, i); self.values[i] = v
static int insert_dyarray(lua_State *L)
{
    DyArray   *self = check_dyarray(L, 1);
    int        idx  = resolve_index(self, luaL_checkint(L, 2));
    lua_Number n    = luaL_checknumber(L, 3);
    return insert_internal(L, self, idx, n);
}

// From:    [ self, v ]
// To:      [ self ]
// Side:    resize(self, self.length + 1), self.values[self.length] = v
static int push_dyarray(lua_State *L)
{
    DyArray   *self = check_dyarray(L, 1); 
    lua_Number n    = luaL_checknumber(L, 2);
    return insert_internal(L, self, self->length + 1, n);
}

// From: [ self ]
// To:   [ self:length() ]
static int length_dyarray(lua_State *L)
{
    lua_pushinteger(L, check_dyarray(L, 1)->length);
    return 1;
}

// From: [ self ]
// To:   [ tostring(self) ]
static int mt_tostring(lua_State *L)
{
    // Lua implements their own format string functionality, %i is unsupported.
    DyArray *self = check_dyarray(L, 1);
    int      len  = self->length;
    int      used = len + 2;
    
    // Can fit all array elements, plus formatters, onto the stack?
    luaL_checkstack(L, used + 1, "Not enough memory to convert array to string");
    lua_pushfstring(L, LIB_NAME "(length = %d, {", len); // [ self, '{' ]
    for (int i = 1; i <= len; i++) {
        lua_pushnumber(L, self->values[i - 1]); // [ self, '{', f]
        
        // Not at last element?
        if (i < len) {
            lua_pushstring(L, ", "); // [ self, '{', f, ", " ]
            lua_concat(L, 2);        // [ self, '{', f .. ", " ]
        }
    }
    lua_pushstring(L, "})"); // [ self, '{', tostring(self), '}' ]
    
    // [ self, '{' .. tostring(self) .. ']' ]
    lua_concat(L, used);
    return 1;
}

// From: [ self, key ]
// To:   [ self[key] ]
static int mt_index(lua_State *L)
{
    switch (lua_type(L, 2)) {
    case LUA_TNUMBER: return get_dyarray(L);
    case LUA_TSTRING: return get_field(L);
    default:          break;
    }

    lua_getglobal(L, "tostring"); // [ self, key, tostring ]
    lua_pushvalue(L, 2);          // [ self, key, tostring, key ]
    lua_call(L, 1, 1);            // [ self, key, tostring(key) ]
    return bad_field(L, lua_tostring(L, -1));
}

static const luaL_reg lib_dyarray[] = {
    {"new",        &new_dyarray},
    {"get",        &get_dyarray},
    {"set",        &set_dyarray},
    {"insert",     &insert_dyarray},
    {"push",       &push_dyarray},
    {"resize",     &resize_dyarray},
    {"length",     &length_dyarray},
    {NULL,         NULL},
};

static const luaL_reg mt_dyarray[] = {
    {"__index",    &mt_index},
    {"__newindex", &set_dyarray},
    {"__tostring", &mt_tostring},
    {NULL,         NULL},
};

LUALIB_API int luaopen_dyarray(lua_State *L)
{
    // See:
    // https://www.lua.org/manual/5.1/manual.html#luaL_newmetatable
    // https://www.lua.org/manual/5.1/manual.html#luaL_register
    // Unique metatable identifier that hopefully does not clash.
    luaL_newmetatable(L, MT_NAME);           // [ mt ]
    luaL_register(L, NULL, mt_dyarray);      // [ mt ], reg. mt_dyarray to mt (top)
    luaL_register(L, LIB_NAME, lib_dyarray); // [ mt, dyarray ], reg. lib_dyarray to _G.dyarray
    
    lua_pushcfunction(L, &dump_table);
    lua_setglobal(L, "dump_table");
    return 0;
}
