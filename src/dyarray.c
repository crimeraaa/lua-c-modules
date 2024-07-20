/**
 * @name    User-Defined Types in C
 * @link    https://www.lua.org/pil/28.html
 *
 * @note    Recall that in Lua, each and every function has it's own "stack frame"
 *          or window into the primary stack. Think of it like a read-write view
 *          or slice into the VM stack.
 *
 *          [0] is the function object itself. However we cannot poke at it.
 *          [1] is the first argument, if applicable.
 *          So on and so forth.
 *
 * @note    In the Lua 5.1 manual, the right-hand-side notation [-o, +p, x] is:
 *
 *          -o:     Number of values popped by the function.
 *          +p:     Number of values pushed by the function.
 *          x:      Type of error, if any, thrown by the function:
 *                  '-':    never throws an error.
 *                  'm':    memory error.
 *                  'v':    purposely thrown.
 *                  'e':    other kind of error.
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
    int         length;   // #Active, also 1 past last written C index.
    int         capacity; // #Allocated, also 1 past last valid C index.
    lua_Number *values;   // Heap-allocated 1D array.
} DyArray;

#define size_of_values(self, n) size_of_array((self)->values, n)
#define size_of_active(self)    size_of_values(self, (self)->length)
#define size_of_total(self)     size_of_values(self, (self)->capacity)

// HELPERS ----------------------------------------------------------------- {{{

/**
 * @brief   May throw if metatable does not match.
 *          `luaL_checkudata()` checks the stack index against the given
 *          metatable name. This name may be found in the registry.
 *
 * @note    As of Lua 5.1 `luaL_checkudata()` throws by itself.
 *          See: https://www.lua.org/manual/5.1/manual.html#7.3
 */
static DyArray *l_checkarg_dyarray(lua_State *L, int argn)
{
    return luaL_checkudata(L, argn, LIB_MTNAME);
}

// Convert a relative Lua 1-based index to an absolute C 0-based index.
static int l_resolve_index(DyArray *self, int i)
{
    return (i < 0) ? self->length + i : i - 1;
}

static int l_checkarg_index(lua_State *L, DyArray *self, int argn)
{
    int i = l_resolve_index(self, luaL_checkint(L, argn));
    luaL_argcheck(L, 0 <= i && i < self->length, argn, "index out of range");
    return i;
}

/**
 * @return  Valid pointer to an element within `self.values`.
 *
 * @note    Assumes stack is: [ [1] = self, [2] = index, ...args ].
 */
static lua_Number *l_poke_value(lua_State *L)
{
    DyArray *self = l_checkarg_dyarray(L, 1);
    return &self->values[l_checkarg_index(L, self, 2)];
}

// Assumes `start` and `stop` are both 0-based indexes.
static lua_Number *c_clear_values(lua_Number *dst, int start, int stop)
{
    DBG_PRINTFLN("clear indexes %d to %d", start, stop);
    for (int i = start; i < stop; i++)
        dst[i] = 0;
    return dst;
}

// Assumes `len` also refers to 1 past the last valid 0-based index.
static lua_Number *c_copy_values(lua_Number *dst, lua_Number *src, int len)
{
    DBG_PRINTFLN("copy indexes 0 to %d", len);
    for (int i = 0; i < len; i++)
        dst[i] = src[i];
    return dst;
}

static int bad_newtype(lua_State *L, const char *tname)
{
    return LIB_ERROR(L, "Cannot create new " LIB_QNAME " from %s", tname);
}

static int bad_index(lua_State *L, int i)
{
    const char *tname = luaL_typename(L, -1);
    return LIB_ERROR(L, "Non-number at index %d (a %s value)", i, tname);
}

static int bad_field(lua_State *L, const char *field)
{
    return LIB_ERROR(L, "Unknown field " LUA_QS, field);
}

// TODO: Check for integer overflow
static int next_power_of_2(int x)
{
    int n = 8;
    while (n < x)
        n *= 2;
    return n;
}

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

// Assumes `_G.tostring` is found at the valid index `fn`.
// This has net 0 stack usage so that the caller can reuse it.
static const char *call_tostring(lua_State *L, int fn_idx, int arg_idx)
{
    const char *s;
    lua_pushvalue(L, fn_idx);  // [ tostring]
    lua_pushvalue(L, arg_idx); // [ tostring, arg ]
    lua_call(L, 1, 1);         // [ tostring(arg) ]
    s = lua_tostring(L, -1);
    lua_pop(L, 1);
    return s;
}

// TODO: Make recursive using memoization
static int c_dump_table(lua_State *L, int t_idx)
{
    lua_pushnil(L); // [ t, nil ] ; set space for key

    // https://www.lua.org/manual/5.1/manual.html#lua_next
    while (lua_next(L, t_idx)) {
        // [ t, k, t[k] ]
        int k_idx = t_idx + 1;
        int v_idx = t_idx + 2;
        printf("[");    print_value(L, k_idx);
        printf("] = "); print_value(L, v_idx);
        printf("\n");

        // [ t, m, k ]
        lua_pop(L, 1);
    }
    return 0;
}

static int dump_table(lua_State *L)
{
    return lua_istable(L, 1) ? c_dump_table(L, 1) : luaL_typerror(L, 1, "table");
}

// }}} -------------------------------------------------------------------------

// METHODS ---------------------------------------------------------------- {{{1

/**
 * @exception lua_newuserdata(): memory
 *
 * @note    Stack usage:    [ 0, +1, m ]
 *          Stack before:   [ ...args ]
 *          Stack after:    [ ...args, self ]
 */
static DyArray *c_new_dyarray(lua_State *L, int len, int cap)
{
    DyArray *self  = lua_newuserdata(L, sizeof(*self)); // [ ...args, self ]

    DBG_PRINTFLN("new " LIB_QNAME " of length %d, capacity %d", len, cap);
    self->length   = len;
    self->capacity = cap;
    self->values   = new_pointer(L, size_of_total(self));

    luaL_getmetatable(L, LIB_MTNAME); // [ ...args, self, mt ]
    lua_setmetatable(L, -2);          // [ ...args, self ] ; setmetatable(self, mt)
    return self;
}

/**
 * @exception <args[1]>:       type
 *            c_new_dyarray(): memory
 *            <for-body>:      index
 *
 * @note    Stack usage:    [ -(0|1), +1, m|v ]
 *          Stack before:   [ data?: number[] ]
 *          Stack after:    [ self: dyarray ]
 *
 * @see     https://www.lua.org/pil/28.1.html
 */
static int new_dyarray(lua_State *L)
{
    DyArray *self;
    int      len, cap;

    switch (lua_type(L, 1)) {
    case LUA_TNONE:
    case LUA_TNIL:
        len = 0;
        break;
    case LUA_TTABLE:
        len = cast_int(lua_objlen(L, 1));
        break;
    case LUA_TUSERDATA:
        // If true: [ data, #data ]
        if (luaL_callmeta(L, 1, "__len")) {
            len = cast_int(lua_tointeger(L, -1));
            lua_pop(L, 1); // [ data, #data ] -> [ data ]
            break;
        }
        // Else fall through
    default:
        return bad_newtype(L, luaL_typename(L, 1));
    }

    cap  = next_power_of_2(len);
    self = c_new_dyarray(L, len, cap); // [ t, self ]
    for (int i = 1; i <= len; i++) {
        lua_pushinteger(L, i); // [ t, self, i ]
        lua_gettable(L, 1);    // [ t, self, t[i] ] ; may call metamethod.
        if (lua_isnumber(L, -1))
            self->values[i - 1] = lua_tonumber(L, -1);
        else
            return bad_index(L, i); // Will also call __gc.
        lua_pop(L, 1); // [ t, self ]
    }
    c_clear_values(self->values, len, cap);
    return 1;
}

/**
 * @exception <args[1]>:       type
 *            c_new_dyarray(): memory
 *
 * @note    Stack usage:    [ -1, +1, m|v ]
 *          Stack before:   [ self: dyarray ]
 *          Stack after:    [ self:copy(): dyarray ]
 */
static int copy_dyarray(lua_State *L)
{
    DyArray *self = l_checkarg_dyarray(L, 1);
    int      len  = self->length;
    int      cap  = self->capacity;
    DyArray *copy = c_new_dyarray(L, len, cap); // [ self, copy ]

    c_copy_values(copy->values, self->values, len);
    c_clear_values(copy->values, len, cap);
    return 1;
}

/**
 * @exception <args[:]>: type
 *            <args[2]>: index
 *
 * @note    Stack usage:    [ -2, +1, v ]
 *          Stack before:   [ self: dyarray, index: integer ]
 *          Stack after:    [ self[index]: number ]
 */
static int get_dyarray(lua_State *L)
{
    lua_pushnumber(L, *l_poke_value(L));
    return 1;
}

/**
 * @exception <args[:]>: type
 *            <return>:  field
 *
 * @note    Stack usage:    [ -2, +1, v ]
 *          Stack before:   [ self: dyarray, key: string ]
 *          Stack after:    [ array[key]: function ]
 *
 */
static int get_field(lua_State *L)
{
    const char *s = luaL_checkstring(L, 2);
    lua_getglobal(L, LIB_NAME); // [ self, key, dyarray ]
    lua_getfield(L, -1, s);     // [ self, key, dyarray, dyarray[key] ]
    return lua_isnil(L, -1) ? bad_field(L, s) : 1;
}

/**
 * @exception <args[:]>: type
 *
 * @note    Stack usage:    [ -3, +1, v ]
 *          Stack before:   [ self: dyarray, i: integer, v: number ]
 *          Stack after:    [ self: dyarray ]
 *          Side effects:   self.values[index] = value
 */
static int set_dyarray(lua_State *L)
{
    *l_poke_value(L) = luaL_checknumber(L, 3);
    lua_pushvalue(L, 1); // [ self, index, value, self ]
    return 1;
}

/**
 * @brief   We assume that the allocator will free the old pointer upon resizing.
 *          We also assume that the allocator will handle reallocation requests
 *          for the same size.
 *
 * @exception resize_pointer(): memory
 *
 * @note    Stack usage:    [ -0, +1, m ]
 *          Stack before:   [ self: dyarray, ...args: any ]
 *          Stack after:    [ self, args, self ]
 */
static int c_resize_dyarray(lua_State *L, DyArray *self, int nlen, int ncap)
{
    size_t      osz = size_of_total(self);
    size_t      nsz = size_of_values(self, ncap);
    lua_Number *tmp = resize_pointer(L, self->values, osz, nsz);
    DBG_PRINTFLN("resize buffer from %d to %d", self->capacity, ncap);

    // If we extended, zero out the new region.
    self->values   = c_clear_values(tmp, self->length, ncap);
    self->length   = nlen;
    self->capacity = ncap;
    lua_pushvalue(L, 1); // [ self, ...args, self ]
    return 1;
}

/**
 * @brief   Assumes `c_idx` is, well, 0-based. Also assumes we verified the size
 *          and such.
 *
 * @note    Stack before:   [ self: dyarray, ...args: any ]
 *          Stack after:    [ self, ...args, self ]
 */
static int c_insert_dyarray(lua_State *L, DyArray *self, int c_idx, lua_Number n)
{
    int len = self->length;
    self->values[c_idx] = n;

    // length refers to 1 past last written C index, so if true we need to update.
    if (c_idx >= len)
        self->length = c_idx + 1;

    lua_pushvalue(L, 1);
    return 1;
}

static int c_remove_dyarray(lua_State *L, DyArray *self, int c_idx)
{
    int        len = self->length;
    lua_Number n   = self->values[c_idx];

    // Move all elements to the right, 1 step to the left.
    for (int i = c_idx; i < len; i++)
        self->values[i] = self->values[i + 1];

    lua_pushnumber(L, n);
    self->length -= 1;
    return 1;
}

/**
 * @exception <args[:]>:  type
 *            (nlen < 0): memory
 *
 * @note    Stack before:   [ self: dyarray, nlen: integer ]
 *          Stack after:    []
 *          Side effects:   self.values = realloc(self.values, nlen)
 */
static int resize_dyarray(lua_State *L)
{
    DyArray *self = l_checkarg_dyarray(L, 1);
    int      nlen = luaL_checkint(L, 2);
    if (nlen < 0)
        return LIB_ERROR(L, "Cannot resize to %d elements", nlen);
    else
        return c_resize_dyarray(L, self, nlen, next_power_of_2(nlen));
}

/**
 * @exception <args[:]>: type
 *            <args[2]>: index
 *
 * @note    Stack usage:    [ -3, +1, v ]
 *          Stack before:   [ self: dyarray, i: integer, v: number ]
 *          Stack after:    [ self: dyarray ]
 *          Side effects:   self.values[i] = v
 *
 * @note    If `i` is not currently in range, we will throw.
 */
static int insert_dyarray(lua_State *L)
{
    DyArray   *self  = l_checkarg_dyarray(L, 1);
    int        c_idx = l_checkarg_index(L, self, 2);
    lua_Number n     = luaL_checknumber(L, 3);
    return c_insert_dyarray(L, self, c_idx, n);
}

/**
 * @exception <args[:]>: type
 *            <args[2]>: index
 *
 * @note    Stack usage:    [ -2, +1, v ]
 *          Stack before:   [ self, i ]
 *          Stack after:    [ self.values[i] ]
 *          Side effects:   table.remove(self.values, i)
 *                          self.length -= 1
 */
static int remove_dyarray(lua_State *L)
{
    DyArray *self  = l_checkarg_dyarray(L, 1);
    int      c_idx = l_checkarg_index(L, self, 2);
    return c_remove_dyarray(L, self, c_idx);
}

// PUSH AND POP ----------------------------------------------------------- {{{2

/**
 * @exception <args[:]>:          type
 *            c_resize_dyarray(): memory
 *
 * @note    Stack usage:    [ -2, +1, m|v ]
 *          Stack before:   [ self, v ]
 *          Stack after:    [ self ]
 *          Side effects:   realloc(self.values, self.length + 1)
 *                          self.values[self.length] = v
 */
static int push_dyarray(lua_State *L)
{
    DyArray   *self = l_checkarg_dyarray(L, 1);
    int        len  = self->length;
    lua_Number n    = luaL_checknumber(L, 2);

    // cap of 4 means last valid C index is 3, and if `len` is 4 that means
    // we need to resize.
    if (len >= self->capacity)
        c_resize_dyarray(L, self, len, next_power_of_2(len));
    return c_insert_dyarray(L, self, len, n);
}

/**
 * @exception len <= 0: index
 *
 * @note    Stack usage:    [ -1, +1, v ]
 *          Stack before:   [ self ]
 *          Stack after:    [ self[self.length] ]
 *          Side effect:    self.length -= 1
 */
static int pop_dyarray(lua_State *L)
{
    DyArray *self = l_checkarg_dyarray(L, 1);
    int      len  = self->length;
    if (len <= 0)
        return LIB_ERROR(L, "Nothing to pop, have %d elements", len);
    return c_remove_dyarray(L, self, len - 1);
}

// 2}}} ------------------------------------------------------------------------

/**
 * @exception <args[1]>: type
 *
 * @note    Stack usage:    [ -1, +1, v ]
 *          Stack before:   [ self ]
 *          Stack after:    [ self:length() ]
 */
static int length_dyarray(lua_State *L)
{
    lua_pushinteger(L, l_checkarg_dyarray(L, 1)->length);
    return 1;
}

// 1}}} ------------------------------------------------------------------------

// METATABLES ------------------------------------------------------------- {{{1

/**
 * @exception <args[1]>:         type
 *            lua_pushfstring(): memory
 *            luaL_add*:         memory
 *
 * @note    Stack usage:    [ -1, +1, m|v ]
 *          Stack before:   [ self ]
 *          Stack after:    [ tostring(self) ]
 */
static int mt_tostring(lua_State *L)
{
    DyArray    *self = l_checkarg_dyarray(L, 1);
    int         len  = self->length;
    luaL_Buffer buf;

    luaL_buffinit(L, &buf);
    lua_pushfstring(L, LIB_MESSAGE("length = %d, values = {", len));
    luaL_addvalue(&buf); // [ self, fmt ] -> [ self ]

    for (int i = 0; i < len; i++) {
        lua_pushnumber(L, self->values[i]);
        luaL_addvalue(&buf); // [ self, self.values[i] ] -> [ self ]
        if (i < len - 1)
            luaL_addstring(&buf, ", ");
    }

    luaL_addchar(&buf, '}');
    luaL_pushresult(&buf);
    return 1;
}

/**
 * @exception <args[2]>: type, index, field
 *
 * @note    Stack usage:  [ -2, +1, v ]
 *          Stack before: [ self: dyarray, key: number|string ]
 *          Stack after:  [ self[key]: number|function ]
 *
*/
static int mt_index(lua_State *L)
{
    switch (lua_type(L, 2)) {
    case LUA_TNUMBER: return get_dyarray(L);
    case LUA_TSTRING: return get_field(L);
    default:          break;
    }

    lua_getglobal(L, "tostring"); // [ self, key, tostring ]
    lua_pushvalue(L, 2);          // [ self, key, tostring, key ]
    return bad_field(L, call_tostring(L, 3, 4));
}

/**
 * @brief   `dyarray.values` is not managed by Lua, we must free it ourselves.
 *          Think of this like a pseudo-destructor.
 *
 * @exception <args[1]>: type
 *
 * @note    Stack usage:    [ -1, 0, - ]
 *          Stack before:   [ self: dyarray ]
 *          Stack after:    []
 *          Side effect:    free(self.values)
 */
static int mt_gc(lua_State *L)
{
    DyArray *self = l_checkarg_dyarray(L, 1);
    DBG_PRINTFLN("free buffer of length %d", self->length);

    // osize > 0 && nsize == 0 invokes the equivalent of free(ptr).
    free_pointer(L, self->values, size_of_total(self));
    return 0;
}

// 1}}} ------------------------------------------------------------------------

static const luaL_Reg lib_fns[] = {
    {"new",         &new_dyarray},
    {"get",         &get_dyarray},
    {"set",         &set_dyarray},

    // Explicit index manipulation
    {"insert",      &insert_dyarray},
    {"remove",      &remove_dyarray},

    // Implicit index manipulation
    {"push",        &push_dyarray},
    {"pop",         &pop_dyarray},

    // Pseudo memory management
    {"resize",      &resize_dyarray},
    {"copy",        &copy_dyarray},
    {"length",      &length_dyarray},
    {NULL,          NULL},
};

static const luaL_Reg mt_fns[] = {
    {"__index",    &mt_index},
    {"__newindex", &set_dyarray},
    {"__tostring", &mt_tostring},
    {"__len",      &length_dyarray}, // In Lua 5.1 this only works for userdata.
    {"__gc",       &mt_gc},
    {NULL,         NULL},
};

LIB_EXPORT int luaopen_dyarray(lua_State *L)
{
    // Intern error message so we don't need to allocate it later on.
    lua_pushliteral(L, LIB_MEMERR);    // [ LIB_MEMERRR ]
    lua_pop(L, 1);                     // []
    lua_pushcfunction(L, &dump_table); // [ dump_table ]
    lua_setglobal(L, "dump_table");    // [] ; _G.dump_table == dump_table

    // See:
    // https://www.lua.org/manual/5.1/manual.html#luaL_newmetatable
    // https://www.lua.org/manual/5.1/manual.html#luaL_register
    luaL_newmetatable(L, LIB_MTNAME);    // [ mt ]
    luaL_register(L, NULL, mt_fns);      // [ mt ], reg(mt, mt_fns)
    luaL_register(L, LIB_NAME, lib_fns); // [ mt, dyarray ], reg(_G.dyarray, lib_fns)

    return 1;
}
