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
#define LIB_NAME "dyarray"
#include "common.h"
#include <string.h>

typedef struct {
    int         length;   // Number of contiguous active elements.
    int         capacity; // Number of total allocated elements.
    lua_Number *values;   // Heap-allocated 1D array.
} DyArray;

#define size_of_values(self, n)  size_of_array((self)->values, n)

#ifdef _DEBUG
#define debug_printf(fmt, ...)      printf("[DEBUG]: " fmt, __VA_ARGS__)
#define debug_printfln(fmt, ...)    debug_printf(fmt "\n", __VA_ARGS__)
#define debug_println(s)            debug_printfln("%s", s)
#else
#define debug_printf(fmt, ...)
#define debug_printfln(fmt, ...)
#define debug_println(s)
#endif

static void print_value(lua_State *L, int i)
{
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
    case LUA_TSTRING: {
        const char *s = lua_tostring(L, i);
        const char  q = (lua_objlen(L, i) == 1) ? '\'' : '\"';
        printf("%c%s%c", q, s, q);
        break;
    }
    default:
        printf("%s(%p)", luaL_typename(L, i), lua_topointer(L, i));
        break;
    }
}

// Adapted from: https://www.lua.org/pil/24.2.3.html
static int dump_stack(lua_State *L)
{
    for (int i = 1, top = lua_gettop(L); i <= top; i++) {
        printf("[%d] ", i);
        print_value(L, i);
        printf("\n");
    }
    printf("\n");
    return 0;
}

// Assumes `_G.tostring` is found at the valid index `fn`.
// static const char *to_string(lua_State *L, int fn, int arg)
// {
//     const char *s;
//     lua_pushvalue(L, fn);  // [ tostring]
//     lua_pushvalue(L, arg); // [ tostring, arg ]
//     lua_call(L, 1, 1);     // [ tostring(arg) ]
//     s = lua_tostring(L, -1);
//     lua_pop(L, 1); // []
//     return s;
// }

// From:    [ t ]
// To:      []
static int dump_table(lua_State *L)
{
    if (!lua_istable(L, 1))
        return luaL_typerror(L, 1, lua_typename(L, LUA_TTABLE));

    lua_pushnil(L); // [ t, nil ] ; Set space for key.

    // https://www.lua.org/manual/5.1/manual.html#lua_next
    while (lua_next(L, 1)) {
        // [ t, k, t[k] ]
        printf("[");    print_value(L, 2);
        printf("] = "); print_value(L, 3);
        printf("\n");
        lua_pop(L, 1); // [ t, k ]
    }
    return 0;
}

static int bad_index(lua_State *L, int i)
{
    const char *tname = luaL_typename(L, -1);
    return luaL_error(L, "non-number at index %d (a %s value)", i, tname);
}

// TODO: Check for integer overflow
static int next_power_of_2(int x)
{
    int n = 8;
    while (n <= x)
        n *= 2;
    return n;
}

// From: [ t ]
// To:   [ self ]
//
// https://www.lua.org/pil/28.1.html
static int new_dyarray(lua_State *L)
{
    DyArray *self;

    luaL_checktype(L, 1, LUA_TTABLE);
    self           = lua_newuserdata(L, sizeof(*self)); // [ t, self ]
    self->length   = cast_int(lua_objlen(L, 1));
    self->capacity = next_power_of_2(self->length);
    self->values   = new_pointer(L, size_of_values(self, self->capacity));

    luaL_getmetatable(L, LIB_MTNAME); // [ t, self, mt ]
    lua_setmetatable(L, -2);          // [ t, self ] ; setmetatable(self, mt)

    for (int i = 1; i <= self->length; i++) {
        lua_rawgeti(L, 1, i); // [ t, self, t[i] ]
        if (lua_isnumber(L, -1))
            self->values[i - 1] = lua_tonumber(L, -1);
        else
            return bad_index(L, i); // Will call __gc if applicable.
        lua_pop(L, 1); // [ t, self ]
    }
    // Zero out uninitialized region.
    for (int i = self->length + 1; i <= self->capacity; i++)
        self->values[i - 1] = 0;
    return 1; // self already on top of stack
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
    return luaL_checkudata(L, argn, LIB_MTNAME);
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
// Does:   self.values[index] = value
static int set_dyarray(lua_State *L)
{
    *get_item(L) = luaL_checknumber(L, 3);
    lua_pushvalue(L, 1); // [ self, index, value, self ]
    return 1;
}

// We assume that the allocator will free the old pointer upon resizing.
//
// From:    [ self, ...args ]
// To:      [ self ]
static int c_resize_dyarray(lua_State *L, DyArray *self, int nlen, int ncap)
{
    size_t      osz = size_of_values(self, self->capacity); // old size.
    size_t      nsz = size_of_values(self, ncap);           // new size.
    lua_Number *tmp = resize_pointer(L, self->values, osz, nsz);
    debug_printfln("resizing DyArray::values from %d to %d", self->capacity, ncap);

    // If we extended, zero out the new region.
    for (int i = self->length + 1; i <= ncap; i++)
        tmp[i - 1] = 0;

    self->values   = tmp;
    self->length   = nlen;
    self->capacity = ncap;
    lua_pushvalue(L, 1); // [ self, ...args, self ]
    return 1;
}

// From:    [ self, nlen ]
// To:      []
// Does:    resize(self, nlen)
static int resize_dyarray(lua_State *L)
{
    DyArray *self = check_dyarray(L, 1);
    int      nlen = luaL_checkint(L, 2);
    if (nlen <= 0)
        return luaL_error(L, "Cannot resize " LIB_NAME_Q " to %d items", nlen);
    else
        return c_resize_dyarray(L, self, nlen, nlen);
}

// From: [ self, ...args ]
// To:   [ self ]
static int c_insert_dyarray(lua_State *L, DyArray *self, int idx, lua_Number n)
{
    // Need to grow?
    if (idx > self->capacity)
        c_resize_dyarray(L, self, idx, next_power_of_2(idx));

    self->values[idx - 1] = n;
    if (idx > self->length)
        self->length = idx;

    lua_pushvalue(L, 1);
    return 1;
}

// From:    [ self, i, v ]
// To:      [ self ]
// Does:    i > self.length ? resize(self, i); self.values[i] = v
static int insert_dyarray(lua_State *L)
{
    DyArray   *self = check_dyarray(L, 1);
    int        idx  = resolve_index(self, luaL_checkint(L, 2));
    lua_Number n    = luaL_checknumber(L, 3);
    return c_insert_dyarray(L, self, idx, n);
}

// From:    [ self, v ]
// To:      [ self ]
// Does:    resize(self, self.length + 1), self.values[self.length] = v
static int push_dyarray(lua_State *L)
{
    DyArray   *self = check_dyarray(L, 1);
    lua_Number n    = luaL_checknumber(L, 2);
    return c_insert_dyarray(L, self, self->length + 1, n);
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
    DyArray *self = check_dyarray(L, 1);
    int      len  = self->length;
    int      used = len + 2;

    // Can fit all array elements, plus formatters, onto the stack?
    luaL_checkstack(L, used + 1, "Not enough memory to convert array to string");

    // In the Lua API format string functionality, %i is unsupported.
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
    lua_concat(L, used);     // [ self, '{' .. tostring(self) .. ']' ]
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

// Since `DyArray::values` is not managed by Lua, we must free it ourselves.
// Think of this like a pseudo-destructor.
//
// From:    [ self ]
// To:      []
// Does:    free(self.values)
static int mt_gc(lua_State *L)
{
    DyArray *self = check_dyarray(L, 1);
    debug_printfln("freeing DyArray::values of length %d", self->length);

    // osize > 0 && nsize == 0 invokes the equivalent of free(ptr).
    free_pointer(L, self->values, size_of_values(self, self->capacity));
    return 0;
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
    {"__gc",       &mt_gc},
    {NULL,         NULL},
};

LIB_EXPORT int luaopen_dyarray(lua_State *L)
{
    // Intern error message so we don't need to allocate it later on.
    lua_pushliteral(L, LIB_MEMERR); // [ LIB_MEMERRR ]
    lua_pop(L, 1);                  // []

    // See:
    // https://www.lua.org/manual/5.1/manual.html#luaL_newmetatable
    // https://www.lua.org/manual/5.1/manual.html#luaL_register
    luaL_newmetatable(L, LIB_MTNAME);        // [ mt ]
    luaL_register(L, NULL, mt_dyarray);      // [ mt ], reg(mt, mt_dyarray)
    luaL_register(L, LIB_NAME, lib_dyarray); // [ mt, dyarray ], reg(_G.dyarray, lib_dyarray)

    lua_pushcfunction(L, &dump_table); // [ mt, dyarray, dump_table ]
    lua_setglobal(L, "dump_table");    // [ mt, dyarray ] ; _G.dump_table == dump_table
    return 0;
}
