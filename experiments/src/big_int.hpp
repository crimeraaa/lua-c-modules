#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <string_view>
#include "common.hpp"

#define BIGINT_BASE 10

struct BigInt {
    using Digit = std::uint8_t;
    using Size  = std::size_t;
    using Index = int;
    using Iter  = Digit*;
    using RevIt = std::reverse_iterator<Iter>;
    
    Digit digits[0x40];
    Size  length;   // Number of active digits. &digits[length] is end().
    Size  capacity; // Number of digits we can hold before needing to reallocate.
};

void   bigint_init(BigInt* self);
BigInt bigint_make_from_string(std::string_view sv);

BigInt::Digit bigint_read_at(BigInt* self, BigInt::Index i);
void bigint_write_at(BigInt* self, BigInt::Index i, BigInt::Digit d);

// let d = 5
// Conceptually:    1234      -> 51234
// Internally:      {4,3,2,1} -> {4,3,2,1,5}
void bigint_push_left(BigInt* self, BigInt::Digit d);

// Conceptually:    1234      -> 234
// Internally:      {4,3,2,1} -> {4,3,2}
BigInt::Digit bigint_pop_left(BigInt* self);

// Conceptually:    1234 -> 12340
// Internally:      1. {4,3,2,1}   -> {4,3,2,1,0}
//                  2. {4,3,2,1,0} -> {4,4,3,2,1}
//                  3. {4,4,3,2,1} -> {0,4,3,2,1}
void bigint_shift_left1(BigInt* self);

// let d = 5
// Conceptually:    1234      -> 12345
// Internally:      {4,3,2,1} -> {5,4,3,2,1}
void bigint_push_right(BigInt* self, BigInt::Digit d);

// Conceptually:    1234         -> 123
// Internally:      1. {4,3,2,1} -> {3,2,1,1}
//                  2. {3,2,1,1} -> {3,2,1}
void bigint_shift_right1(BigInt* self);

// Conceptually:    1234 -> 123, return 4
// Internally:      d = digits[0], rshift1(digits)
BigInt::Digit bigint_pop_right(BigInt* self);

void bigint_print(BigInt* self);

// Read-write forward iterators.
BigInt::Iter bigint_begin(BigInt* self);
BigInt::Iter bigint_end(BigInt* self);

// Read-write reverse iterators.
BigInt::RevIt bigint_rbegin(BigInt* self);
BigInt::RevIt bigint_rend(BigInt* self);

// Relative index (positive, zero or negative) to absolute index (positive).
BigInt::Size bigint_resolve_index(BigInt* self, BigInt::Index i);

// Ensure `d` is a decimal digit (NOT a character!)
bool bigint_check_digit(BigInt::Digit d);
bool bigint_check_index(BigInt* self, BigInt::Index i);
