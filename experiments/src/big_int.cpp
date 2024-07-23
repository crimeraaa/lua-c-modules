#include <cctype>
#include <string_view>
#include <iterator>
#include "common.hpp"
#include "big_int.hpp"

int main()
{
    static const BigInt BIGINT_TEST = bigint_make_from_string("1234");
    BigInt a = BIGINT_TEST;   // 1234
    bigint_push_left(&a, 5);  // 51234
    bigint_print(&a);
    
    a = BIGINT_TEST;          // 1234
    bigint_push_right(&a, 0); // 12340
    bigint_print(&a);
    
    a = BIGINT_TEST;          // 1234
    bigint_shift_left1(&a);   // 12340
    bigint_print(&a);

    a = BIGINT_TEST;          // 1234
    bigint_shift_right1(&a);  // 123
    bigint_print(&a);
    
    a = BIGINT_TEST;          // 1234
    bigint_pop_left(&a);      // 234
    bigint_print(&a);

    a = BIGINT_TEST;          // 1234
    bigint_pop_right(&a);     // 123
    bigint_print(&a);
    
    a = BIGINT_TEST;          // 1234
    bigint_pop_left(&a);      // 234
    bigint_push_right(&a, 5); // 2345
    bigint_shift_left1(&a);   // 23450
    bigint_push_left(&a, 7);  // 723450
    bigint_push_left(&a, 10); // error
    bigint_print(&a);
    return 0;
}

void bigint_init(BigInt* self)
{
    self->length   = 0;
    self->capacity = sizeof(self->digits);
    std::fill(bigint_begin(self), bigint_end(self), 0); // memset buffer to 0
}

BigInt bigint_make_from_string(std::string_view sv)
{
    BigInt res;
    bigint_init(&res);
    // Iterate string right to left, but push to self left to right.
    for (auto it = sv.rbegin(); it != sv.rend(); it++) {
        char ch = *it;
        if (std::isspace(ch) || ch == '_')
            continue;
        bigint_push_left(&res, ch - '0');
    }
    return res;
}

// --- DIRECTIONAL MANIPULATION ------------------------------------------- {{{1

// --- LEFT --------------------------------------------------------------- {{{2

void bigint_push_left(BigInt* self, BigInt::Digit d)
{
    // digits[capacity] is not readable or writable.
    if (self->length >= self->capacity) {
        DEBUG_PRINTFLN("Need resize from length = %zu, capacity %zu",
                       self->length, self->capacity);
        return;
    }
    if (bigint_check_digit(d)) {
        DEBUG_PRINTFLN("digits[%zu] = %u, length++", self->length, d);
        // Don't use `bigint_write_at()` since that does a bounds check.
        self->digits[self->length] = d;
        self->length += 1;
    } else {
        DEBUG_PRINTFLN("Cannot left-push digit %u", d);
    }
}

BigInt::Digit bigint_pop_left(BigInt* self)
{
    if (self->length == 0) {
        DEBUG_PRINTLN("Cannot left-pop digits[0], return 0");
        return 0;
    }
    // Don't use `bigint_read_at()` since that does a bounds check.
    BigInt::Size  i = self->length - 1;
    BigInt::Digit d = self->digits[i];
    DEBUG_PRINTFLN("return digits[%zu], length--", i);
    self->length -= 1;
    return d;
}

void bigint_shift_left1(BigInt* self)
{
    DEBUG_PRINTFLN("lshift(length = %zu), length++", self->length);

    // TODO: Fix, is unsafe if length >= capacity
    // {4,3,2,1}   -> {4,3,2,1,0}
    self->length += 1;

    // {4,3,2,1,0} -> {4,4,3,2,1}
    for (BigInt::Size i = self->length - 1; i != 0; i--)
        self->digits[i] = self->digits[i - 1];
    
    // {4,4,3,2,1} -> {0,4,3,2,1}
    self->digits[0] = 0;
}

// 2}}} ------------------------------------------------------------------------

// --- RIGHT -------------------------------------------------------------- {{{2

void bigint_push_right(BigInt* self, BigInt::Digit d)
{
    if (bigint_check_digit(d)) {
        DEBUG_PRINTFLN("shift_left1(), digits[0] = %u", d);
        bigint_shift_left1(self); 
        self->digits[0] = d;
    } else {
        DEBUG_PRINTFLN("Cannot right-push digit %u", d);
    }
}

BigInt::Digit bigint_pop_right(BigInt* self)
{
    // Nothing to pop?
    if (self->length == 0) {
        DEBUG_PRINTLN("Cannot right-pop digits[0], return 0");
        return 0;
    } else {
        DEBUG_PRINTLN("return digits[0], shift_right1()");
    }
    BigInt::Digit d = self->digits[0];
    bigint_shift_right1(self);
    return d;
}

void bigint_shift_right1(BigInt* self)
{
    DEBUG_PRINTFLN("rshift(length = %zu), length--", self->length);
    
    // {4,3,2,1} -> {3,2,1,1}
    for (BigInt::Size i = 0; i < self->length; i++)
        self->digits[i] = self->digits[i + 1];
    // {3,2,1,1} -> {3,2,1}
    self->length -= 1;
    
}

// 2}}} ------------------------------------------------------------------------

// 1}}} ------------------------------------------------------------------------

BigInt::Digit bigint_read_at(BigInt* self, BigInt::Index i)
{
    if (bigint_check_index(self, i))
        return self->digits[bigint_resolve_index(self, i)];
    else
        return 0;
}

void bigint_write_at(BigInt* self, BigInt::Index i, BigInt::Digit d)
{
    if (bigint_check_index(self, i))
        self->digits[bigint_resolve_index(self, i)] = d;
}

// --- ITERATORS ---------------------------------------------------------- {{{1

BigInt::Iter bigint_begin(BigInt* self)
{
    return &self->digits[0];
}

BigInt::Iter bigint_end(BigInt* self)
{
    return self->digits + self->length;
}

BigInt::RevIt bigint_rbegin(BigInt* self)
{
    return BigInt::RevIt(bigint_end(self));
}

BigInt::RevIt bigint_rend(BigInt* self)
{
    return BigInt::RevIt(bigint_begin(self));
}

// 1}}} ------------------------------------------------------------------------

// --- UTILITY ------------------------------------------------------------ {{{1

BigInt::Size bigint_resolve_index(BigInt* self, BigInt::Index i)
{
    return (i >= 0) ? i : self->length - i;
}

bool bigint_check_digit(BigInt::Digit d)
{
    return 0 <= d && d < BIGINT_BASE;
}

bool bigint_check_index(BigInt* self, BigInt::Index i)
{
    BigInt::Size ri = bigint_resolve_index(self, i);
    return 0 <= ri && ri < self->capacity;
}

void bigint_print(BigInt* self)
{
    BigInt::Size len = self->length;
    BigInt::Size cap = self->capacity;

    std::printf("digits[:%zu] = ", len);
    for (BigInt::RevIt it = bigint_rbegin(self); it != bigint_rend(self); it++)
        std::printf("%u", *it);
    std::printf(", length = %zu, capacity = %zu\n", len, cap);
    
#ifdef BIGINT_DEBUG
    std::printf("\n");
#endif
}

// 1}}} ------------------------------------------------------------------------
