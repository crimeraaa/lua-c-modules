#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <string_view>
#include "common.hpp"

#define BIGINT_BASE 10

struct BigInt {
    using Digit      = std::uint8_t;
    using Index      = std::size_t;
    using MutFwdIt   = Digit*;
    using MutRevIt   = std::reverse_iterator<MutFwdIt>;
    using ConstFwdIt = const Digit*;
    using ConstRevIt = std::reverse_iterator<ConstFwdIt>;

    struct Pair {
        BigInt::Index index;
        BigInt::Digit digit;
    };
    
    Digit digits[0x40];
    Index length;   // Number of active digits. &digits[length] is end().
    Index capacity; // Number of digits we can hold before needing to reallocate.
};

// --- USER-FACING -------------------------------------------------------------

BigInt bigint_create();
BigInt bigint_make_from_string(std::string_view sv);
void   bigint_add_at(BigInt& self, BigInt::Index i, BigInt::Digit d);

// -----------------------------------------------------------------------------

// --- DIRECTIONAL MANIPULATION ------------------------------------------------

// --- LEFT --------------------------------------------------------------------

// let d = 5
// Conceptually:    1234      -> 51234
// Internally:      {4,3,2,1} -> {4,3,2,1,5}
void bigint_push_left(BigInt& self, BigInt::Digit d);

// Conceptually:    1234      -> 234
// Internally:      {4,3,2,1} -> {4,3,2}
BigInt::Digit bigint_pop_left(BigInt& self);

// Conceptually:    1234 -> 12340
// Internally:      1. {4,3,2,1}   -> {4,3,2,1,0}
//                  2. {4,3,2,1,0} -> {4,4,3,2,1}
//                  3. {4,4,3,2,1} -> {0,4,3,2,1}
void bigint_shift_left1(BigInt& self);

// -----------------------------------------------------------------------------

// --- RIGHT -------------------------------------------------------------------

// let d = 5
// Conceptually:    1234      -> 12345
// Internally:      {4,3,2,1} -> {5,4,3,2,1}
void bigint_push_right(BigInt& self, BigInt::Digit d);

// Conceptually:    1234         -> 123
// Internally:      1. {4,3,2,1} -> {3,2,1,1}
//                  2. {3,2,1,1} -> {3,2,1}
void bigint_shift_right1(BigInt& self);

// Conceptually:    1234 -> 123, return 4
// Internally:      d = digits[0], rshift1(digits)
BigInt::Digit bigint_pop_right(BigInt& self);

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

// --- ITERATORS ---------------------------------------------------------------

BigInt::MutFwdIt   bigint_begin(BigInt& self);
BigInt::MutFwdIt   bigint_end(BigInt& self);
BigInt::MutRevIt   bigint_rbegin(BigInt& self);
BigInt::MutRevIt   bigint_rend(BigInt& self);

BigInt::ConstFwdIt bigint_begin(const BigInt& self);
BigInt::ConstFwdIt bigint_end(const BigInt& self);
BigInt::ConstRevIt bigint_rbegin(const BigInt& self);
BigInt::ConstRevIt bigint_rend(const BigInt& self);

// -----------------------------------------------------------------------------

// --- UTILITY -----------------------------------------------------------------

BigInt::Digit bigint_read_at(const BigInt& self, BigInt::Index i);
void bigint_write_at(BigInt& self, BigInt::Index i, BigInt::Digit d);
void bigint_print(const BigInt& self);
bool bigint_check_digit(BigInt::Digit d);
bool bigint_check_index(const BigInt& self, BigInt::Index i);

// -----------------------------------------------------------------------------
