#include <lua.h>
#include <lauxlib.h>

#ifndef LIB_NAME
#error Please define LIB_NAME as the desired library name before including.
#endif

#define X__STRINGIFY(x) #x
#define STRINGIFY(x)    X__STRINGIFY(x)
#define LIB_NAME_Q      LUA_QL(LIB_NAME)
#define LIB_MTNAME      "C_Modules" LIB_NAME
#define LIB_MEMERR      "Out of memory (module " LIB_NAME_Q ")"

#define LIB_MESSAGE(fmt, ...)   LIB_NAME ": " fmt, __VA_ARGS__
#define LIB_ERROR(L, fmt, ...)  luaL_error((L), LIB_MESSAGE(fmt, __VA_ARGS__))

#ifdef _DEBUG
#define DBG_PRINTF(fmt, ...)    printf("[DEBUG] " LIB_NAME ": " fmt, __VA_ARGS__)
#define DBG_PRINTFLN(fmt, ...)  DBG_PRINTF(fmt "\n", __VA_ARGS__)
#define DBG_PRINTLN(s)          DBG_PRINTFLN("%s", s)
#else
#define DBG_PRINTF(fmt, ...)
#define DBG_PRINTFLN(fmt, ...)
#define DBG_PRINTLN(s)
#endif


#ifdef _WIN32

// Windows needs to explicitly export symbols when building `.dll`.
#define LIB_EXPORT  __declspec(dllexport)

#else  // _WIN32 not defined.

// GCC-like compilers usually export non-static symbols when building `.so`.
#define LIB_EXPORT

#endif // _WIN32

#ifndef __cplusplus
#define cast(T, expr)   ((T)(expr))
#else  // __cplusplus defined.
#define cast(T, expr)   static_cast<T>(expr)
#endif // __cplusplus

#define cast_int(expr)      cast(int, expr)
#define size_of_array(T, n) (sizeof((T)[0]) * (n))

static inline void *resize_pointer(lua_State *L, void *hint, size_t osz, size_t nsz)
{
    void     *ctx;
    lua_Alloc af  = lua_getallocf(L, &ctx);
    void     *ptr = af(ctx, hint, osz, nsz);

    // Non-zero allocation request failed?
    if (nsz != 0 && ptr == NULL)
        luaL_error(L, LIB_MEMERR);
    return ptr;
}

#define new_pointer(L, sz)          resize_pointer(L, NULL, 0, sz)
#define free_pointer(L, ptr, sz)    resize_pointer(L, ptr, sz, 0)
