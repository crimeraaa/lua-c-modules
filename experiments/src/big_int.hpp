#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <string_view>
#include "common.hpp"

namespace big_int {
    static constexpr const int DIGIT_BASE = 10;

    using Digit      = std::uint8_t;
    using Index      = std::size_t;
    using MutFwdIt   = Digit*;
    using MutRevIt   = std::reverse_iterator<MutFwdIt>;
    using ConstFwdIt = const Digit*;
    using ConstRevIt = std::reverse_iterator<ConstFwdIt>;

    struct Pair {
        Index index;
        Digit digit;
    };
        
    struct BigInt {
        Digit digits[0x40];
        Index length;   // Number of active digits. &digits[length] is end().
        Index capacity; // Number of digits we can hold before realloc.
    };

    // --- USER-FACING ---------------------------------------------------------

    BigInt create();
    BigInt copy_string(std::string_view sv);
    void   add(BigInt& self, int n);
    void   add_at(BigInt& self, Index i, Digit d);

    // -------------------------------------------------------------------------

    // --- DIRECTIONAL MANIPULATION --------------------------------------------

    // --- LEFT ----------------------------------------------------------------

    // let d = 5
    // Conceptually:    1234      -> 51234
    // Internally:      {4,3,2,1} -> {4,3,2,1,5}
    bool push_left(BigInt& self, Digit d);

    // Conceptually:    1234      -> 234
    // Internally:      {4,3,2,1} -> {4,3,2}
    Digit pop_left(BigInt& self);

    // This function may resize the buffer.
    // Conceptually:    1234 -> 12340
    // Internally:      1. {4,3,2,1}   -> {4,3,2,1,0}
    //                  2. {4,3,2,1,0} -> {4,4,3,2,1}
    //                  3. {4,4,3,2,1} -> {0,4,3,2,1}
    bool shift_left1(BigInt& self);

    // -------------------------------------------------------------------------

    // --- RIGHT ---------------------------------------------------------------

    // let d = 5
    // Conceptually:    1234      -> 12345
    // Internally:      {4,3,2,1} -> {5,4,3,2,1}
    void push_right(BigInt& self, Digit d);

    // Since this function does not resize the buffer it can never fail.
    // Conceptually:    1234         -> 123
    // Internally:      1. {4,3,2,1} -> {3,2,1,1}
    //                  2. {3,2,1,1} -> {3,2,1}
    void shift_right1(BigInt& self);

    // Conceptually:    1234 -> 123, return 4
    // Internally:      d = digits[0], rshift1(digits)
    Digit pop_right(BigInt& self);

    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------

    // --- ITERATORS -----------------------------------------------------------

    MutFwdIt   begin(BigInt& self);
    MutFwdIt   end(BigInt& self);
    MutRevIt   rbegin(BigInt& self);
    MutRevIt   rend(BigInt& self);

    ConstFwdIt begin(const BigInt& self);
    ConstFwdIt end(const BigInt& self);
    ConstRevIt rbegin(const BigInt& self);
    ConstRevIt rend(const BigInt& self);

    // -------------------------------------------------------------------------

    // --- UTILITY -------------------------------------------------------------

    Digit read_at(const BigInt& self, Index i);
    bool write_at(BigInt& self, Index i, Digit d);
    void print(const BigInt& self);
    bool check_digit(Digit d);
    bool check_index(const BigInt& self, Index i);

    // -------------------------------------------------------------------------
};



