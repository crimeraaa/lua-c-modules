// needed then define LUA_API as export dll
#define LUA_BUILD_AS_DLL
#define LUA_LIB
#include <lua.h>
#include <lauxlib.h>

#define HELLO_VARIABLE  "_HELLO"
#define HELLO_MESSAGE   "Hi mom!"

// Stack before: -
// Stack after: "Hi mom!"
int say_hello(lua_State *L)
{
    lua_getglobal(L, HELLO_VARIABLE);
    return 1;
}

static const struct luaL_Reg functions[] = {
    {"say_hello", &say_hello},
    {NULL, NULL},
};

// Need to be exposed!
// `loadlib("hello")` implicitly searches for `luaopen_hello`.
// Note the form: `luaopen_<NAME>`.
int LUALIB_API luaopen_hello(lua_State *L)
{
    // (lua_State*, libname: const char*, libfns: luaL_Reg[])
    luaL_register(L, "hello", functions);

    // (L: lua_State, msg: const char*)
    lua_pushstring(L, HELLO_MESSAGE);

    // (L: lua_State, key: const char*)
    // Implicitly uses the value at the top of the stack.
    // Set a global identifier `key` to said value.
    lua_setglobal(L, HELLO_VARIABLE);
    return 1;
}
