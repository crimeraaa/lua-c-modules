#pragma once

#include <cstdlib>
#include <stdexcept>

#define array_size(T, N)    (sizeof((T)[0]) * (N))

#ifdef BIGINT_DEBUG

// In `"bigint_*"`, extract `*` only.
#define DEBUG_PRINTF(fmt, ...) \
    std::fprintf(stderr, "%s(): " fmt, &__func__[7], __VA_ARGS__)

#else // _DEBUG not defined.
      //
#define DEBUG_PRINTF(fmt, ...)

#endif // _DEBUG

#define DEBUG_PRINTFLN(fmt, ...)    DEBUG_PRINTF(fmt "\n", __VA_ARGS__)
#define DEBUG_PRINTLN(msg)          DEBUG_PRINTFLN("%s", msg)

class Allocator {
public:
    using Size = std::size_t;
    using Func = void*(*)(void* prev, Size osz, Size nsz, void* ctx);
    
    Allocator(Func fn, void* ctx = nullptr)
    : m_allocate{fn}
    , m_context{ctx}
    {}
    
    template<class T>
    T* allocate(Size len = 1, Size extra = 0)
    {
        return reallocate<T>(nullptr, 0, sizeof(T) * len + extra);
    }
    
    template<class T>
    T* reallocate(T* prev, Size olen, Size nlen)
    {
        size_t osz = sizeof(T) * olen;
        size_t nsz = sizeof(T) * nlen;
        void  *ptr = m_allocate(nullptr, osz, nsz, m_context);
        // Non-zero allocation request failed?
        if (!ptr && nsz != 0)
            throw std::bad_alloc();
        return static_cast<T*>(ptr);
    }
    
    template<class T>
    void deallocate(T* ptr, Size len = 1, Size extra = 0)
    {
        reallocate<T>(ptr, sizeof(T) * len + extra, 0);
    }

private: 
    Func  m_allocate;
    void* m_context;
};
