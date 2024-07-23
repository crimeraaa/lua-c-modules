#include <cctype>
#include "common.hpp"
#include "big_int.hpp"

static const BigInt BIGINT_TEST = bigint_make_from_string("1234");

static void basic_tests()
{
    BigInt a = BIGINT_TEST;  // 1234
    bigint_push_left(a, 5);  // 51234
    bigint_print(a);
    
    a = BIGINT_TEST;         // 1234
    bigint_push_right(a, 0); // 12340
    bigint_print(a);
    
    a = BIGINT_TEST;         // 1234
    bigint_shift_left1(a);   // 12340
    bigint_print(a);

    a = BIGINT_TEST;         // 1234
    bigint_shift_right1(a);  // 123
    bigint_print(a);
    
    a = BIGINT_TEST;         // 1234
    bigint_pop_left(a);      // 234
    bigint_print(a);

    a = BIGINT_TEST;         // 1234
    bigint_pop_right(a);     // 123
    bigint_print(a);
}

static void bufmanip_tests()
{
    BigInt a = BIGINT_TEST;  // 1234
    bigint_pop_left(a);      // 234
    bigint_push_right(a, 5); // 2345
    bigint_shift_left1(a);   // 23450
    bigint_push_left(a, 7);  // 723450
    bigint_push_left(a, 10); // error
    bigint_print(a);
    
    printf("a[0]  = %u\n", bigint_read_at(a, 0));  // digits[0] = 0
    printf("a[1]  = %u\n", bigint_read_at(a, 1));  // digits[1] = 5
    printf("a[-1] = %u\n", bigint_read_at(a, a.length - 1)); // digits[-1] = 7

    bigint_write_at(a, 0, 1);  // 723451
    bigint_write_at(a, 1, 6);  // 723461
    bigint_write_at(a, a.length - 1, 8); // 823461
    bigint_write_at(a, a.length - 2, 3); // 833461
    bigint_write_at(a, 1000, 9); // error
    bigint_print(a);
    
    bigint_write_at(a, 8, 4);  // 400833461
    bigint_print(a);
}

int main()
{
    basic_tests();
    bufmanip_tests();
    BigInt a = bigint_make_from_string("12");
    bigint_add_at(a, 0, 1);             //   13
    bigint_add_at(a, 0, 27);            //   40
    bigint_add_at(a, a.length - 1, 6);  //  100
    bigint_add_at(a, a.length - 1, 19); // 2000
    bigint_add_at(a, 10000, 9);         // error
    return 0;
}

// --- USER-FACING -------------------------------------------------------- {{{1

BigInt bigint_create()
{
    BigInt self;
    self.length   = 0;
    self.capacity = sizeof(self.digits) / sizeof(self.digits[0]);
    std::fill_n(bigint_begin(self), self.capacity, 0); // memset buffer to 0
    return self;
}

BigInt bigint_make_from_string(std::string_view sv)
{
    BigInt self = bigint_create();
    // Iterate string right to left, but push to self left to right.
    for (auto it = sv.rbegin(); it != sv.rend(); it++) {
        char ch = *it;
        if (std::isspace(ch) || ch == '_')
            continue;
        bigint_push_left(self, ch - '0');
    }
    return self;
}

// --- ARITHMETIC --------------------------------------------------------- {{{2

void bigint_add_at(BigInt& self, BigInt::Index i, BigInt::Digit d)
{
    BigInt::Pair prev{i, d};
    int sum;
    int carry = 1; // Initialize to some nonzero so we can begin the loop.
    while (carry != 0) {
        sum   = bigint_read_at(self, prev.index) + prev.digit;
        carry = 0;
        if (sum >= BIGINT_BASE) {
            std::div_t res = std::div(sum, BIGINT_BASE);
            carry = res.quot;
            sum   = res.rem;
        }
        DEBUG_PRINTFLN("digits[%zu] = %u + %i, sum = %u, carry = %u",
                       prev.index,
                       bigint_read_at(self, prev.index),
                       prev.digit,
                       sum,
                       carry);
        bigint_write_at(self, prev.index, sum);
        prev.index += 1;
        prev.digit = carry;
    }

#ifdef BIGINT_DEBUG
    bigint_print(self);
#endif
}

// 2}}} ------------------------------------------------------------------------

// 1}}} ------------------------------------------------------------------------

// --- DIRECTIONAL MANIPULATION ------------------------------------------- {{{1

// --- LEFT --------------------------------------------------------------- {{{2

void bigint_push_left(BigInt& self, BigInt::Digit d)
{
    // digits[capacity] is not readable or writable.
    if (self.length >= self.capacity) {
        DEBUG_PRINTFLN("Need resize from length = %zu, capacity %zu",
                       self.length,
                       self.capacity);
    } else if (!bigint_check_digit(d)) {
        DEBUG_PRINTFLN("Cannot left-push digit %u", d);
    } else {
        DEBUG_PRINTFLN("digits[%zu] = %u, length++", self.length, d);
        // Don't use `bigint_write_at()` since that does a bounds check.
        self.digits[self.length] = d;
        self.length += 1;
    }
}

BigInt::Digit bigint_pop_left(BigInt& self)
{
    if (self.length == 0) {
        DEBUG_PRINTLN("Cannot left-pop digits[0], return 0");
        return 0;
    } else {
        // Don't use `bigint_read_at()` since that does a bounds check.
        BigInt::Index i = self.length - 1;
        BigInt::Digit d = self.digits[i];
        DEBUG_PRINTFLN("return digits[%zu], length--", i);
        self.length -= 1;
        return d;
    }
}

void bigint_shift_left1(BigInt& self)
{
    DEBUG_PRINTFLN("lshift(length = %zu), length++", self.length);

    // TODO: Fix, is unsafe if length >= capacity
    // {4,3,2,1}   -> {4,3,2,1,0}
    self.length += 1;

    // {4,3,2,1,0} -> {4,4,3,2,1}
    for (BigInt::Index i = self.length - 1; i != 0; i--)
        self.digits[i] = self.digits[i - 1];
    
    // {4,4,3,2,1} -> {0,4,3,2,1}
    self.digits[0] = 0;
}

// 2}}} ------------------------------------------------------------------------

// --- RIGHT -------------------------------------------------------------- {{{2

void bigint_push_right(BigInt& self, BigInt::Digit d)
{
    if (bigint_check_digit(d)) {
        DEBUG_PRINTFLN("shift_left1(), digits[0] = %u", d);
        bigint_shift_left1(self); 
        self.digits[0] = d;
    } else {
        DEBUG_PRINTFLN("Cannot right-push digit %u", d);
    }
}

BigInt::Digit bigint_pop_right(BigInt& self)
{
    // Nothing to pop?
    if (self.length == 0) {
        DEBUG_PRINTLN("Cannot right-pop digits[0], return 0");
        return 0;
    } else {
        DEBUG_PRINTLN("return digits[0], shift_right1()");
        BigInt::Digit d = self.digits[0];
        bigint_shift_right1(self);
        return d;
    }
}

void bigint_shift_right1(BigInt& self)
{
    DEBUG_PRINTFLN("rshift(length = %zu), length--", self.length);
    
    // {4,3,2,1} -> {3,2,1,1}
    for (BigInt::Index i = 0; i < self.length; i++)
        self.digits[i] = self.digits[i + 1];
    // {3,2,1,1} -> {3,2,1}
    self.length -= 1;
    
}

// 2}}} ------------------------------------------------------------------------

// 1}}} ------------------------------------------------------------------------

// --- ITERATORS ---------------------------------------------------------- {{{1

BigInt::MutFwdIt bigint_begin(BigInt& self)
{
    return &self.digits[0];
}

BigInt::MutFwdIt bigint_end(BigInt& self)
{
    return self.digits + self.length;
}

BigInt::ConstFwdIt bigint_begin(const BigInt& self)
{
    return &self.digits[0]; 
}

BigInt::ConstFwdIt bigint_end(const BigInt& self)
{
    return self.digits + self.length;
}

BigInt::MutRevIt bigint_rbegin(BigInt& self)
{
    return BigInt::MutRevIt(bigint_end(self));
}

BigInt::MutRevIt bigint_rend(BigInt& self)
{
    return BigInt::MutRevIt(bigint_begin(self));
}

BigInt::ConstRevIt bigint_rbegin(const BigInt& self)
{
    return BigInt::ConstRevIt(bigint_end(self));
}

BigInt::ConstRevIt bigint_rend(const BigInt& self)
{
    return BigInt::ConstRevIt(bigint_begin(self));
}

// 1}}} ------------------------------------------------------------------------

// --- UTILITY ------------------------------------------------------------ {{{1

BigInt::Digit bigint_read_at(const BigInt& self, BigInt::Index i)
{
    // Conceptually, 00001234 == 1234, so 1234[7] should be 0 even if 1234
    // only allocated for 4 digits.
    if (bigint_check_index(self, i))
        return self.digits[i];
    else
        return 0;
}

void bigint_write_at(BigInt& self, BigInt::Index i, BigInt::Digit d)
{
    if (bigint_check_index(self, i)) {
        DEBUG_PRINTFLN("digits[%zu] = %u", i, d);
        self.digits[i] = d;
        
        // We assume that all unused but valid regions are set to 0.
        if (i >= self.length)
            self.length = i + 1;
    } else {
        // TODO resize buffer as needed and zero out newly extended regions
        DEBUG_PRINTFLN("invalid index %zu for digits[:%zu]", i, self.capacity);
    }
}

void bigint_print(const BigInt& self)
{
    std::printf("digits[:%zu] = ", self.length);
    for (auto it = bigint_rbegin(self); it != bigint_rend(self); it++)
        std::printf("%u", *it);
    std::printf(", length = %zu, capacity = %zu\n", self.length, self.capacity);
    
#ifdef BIGINT_DEBUG
    std::printf("\n");
#endif
}

bool bigint_check_digit(BigInt::Digit d)
{
    return 0 <= d && d < BIGINT_BASE;
}

bool bigint_check_index(const BigInt& self, BigInt::Index i)
{
    return 0 <= i && i < self.capacity;
}

// 1}}} ------------------------------------------------------------------------
