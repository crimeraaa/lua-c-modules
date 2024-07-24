#include <cctype>
#include "common.hpp"
#include "big_int.hpp"

static const big_int::BigInt BIGINT_TEST = big_int::copy_string("1234");

static void basic_tests()
{
    big_int::BigInt a = BIGINT_TEST;  // 1234
    big_int::push_left(a, 5);  // 51234
    big_int::print(a);
    
    a = BIGINT_TEST;           // 1234
    big_int::push_right(a, 0); // 12340
    big_int::print(a);
    
    a = BIGINT_TEST;           // 1234
    big_int::shift_left1(a);   // 12340
    big_int::print(a);

    a = BIGINT_TEST;           // 1234
    big_int::shift_right1(a);  // 123
    big_int::print(a);
    
    a = BIGINT_TEST;         // 1234
    big_int::pop_left(a);      // 234
    big_int::print(a);

    a = BIGINT_TEST;         // 1234
    big_int::pop_right(a);     // 123
    big_int::print(a);
}

static void bufmanip_tests()
{
    big_int::BigInt a = BIGINT_TEST;  // 1234
    big_int::pop_left(a);      // 234
    big_int::push_right(a, 5); // 2345
    big_int::shift_left1(a);   // 23450
    big_int::push_left(a, 7);  // 723450
    big_int::push_left(a, 10); // error
    big_int::print(a);
    
    printf("a[0]  = %u\n", big_int::read_at(a, 0));  // digits[0] = 0
    printf("a[1]  = %u\n", big_int::read_at(a, 1));  // digits[1] = 5
    printf("a[-1] = %u\n", big_int::read_at(a, a.length - 1)); // digits[-1] = 7

    big_int::write_at(a, 0, 1);  // 723451
    big_int::write_at(a, 1, 6);  // 723461
    big_int::write_at(a, a.length - 1, 8); // 823461
    big_int::write_at(a, a.length - 2, 3); // 833461
    big_int::write_at(a, 1000, 9); // error
    big_int::print(a);
    
    big_int::write_at(a, 8, 4);  // 400833461
    big_int::print(a);
}

static void add_at_tests()
{
    basic_tests();
    bufmanip_tests();
    big_int::BigInt a = big_int::copy_string("12");
    big_int::add_at(a, 0, 1);             // 13
    big_int::add_at(a, 0, 27);            // error
    big_int::add_at(a, a.length - 1, 6);  // 73
    big_int::add_at(a, a.length - 1, 19); // error
    big_int::add_at(a, 10000, 9);         // error
    big_int::add_at(a, 0, 7);             // 80
}

int main()
{
    add_at_tests();
    big_int::BigInt a = big_int::copy_string("1234");
    big_int::add(a, 1234);  // 2468
    big_int::add(a, 2);     // 2470
    return 0;
}

// --- USER-FACING -------------------------------------------------------- {{{1

big_int::BigInt big_int::create()
{
    BigInt self;
    self.length   = 0;
    self.capacity = sizeof(self.digits) / sizeof(self.digits[0]);
    std::fill_n(begin(self), self.capacity, 0); // memset buffer to 0
    return self;
}

big_int::BigInt big_int::copy_string(std::string_view sv)
{
    BigInt self = create();
    // Iterate string right to left, but push to self left to right.
    for (auto it = sv.rbegin(); it != sv.rend(); it++) {
        char ch = *it;
        if (std::isspace(ch) || ch == '_')
            continue;
        push_left(self, ch - '0');
    }
    return self;
}

// --- ARITHMETIC --------------------------------------------------------- {{{2

void big_int::add(BigInt& self, int n)
{
    if (n < 0) {
        DEBUG_PRINTFLN("Adding negative numbers (%i) not yet supported", n);
        return;
    }
    // Add 1 digit at a time, going from lowest place value to highest.
    int  next = n;
    Pair cur{0, 0};
    
    // If `n` is 0 to begin with then we don't need to do anything.
    while (next != 0) {
        cur.digit = next % DIGIT_BASE;
        next     /= DIGIT_BASE;
        DEBUG_PRINTFLN("digits[%zu] = %u + %u, next = %i",
                       cur.index,
                       read_at(self, cur.index),
                       cur.digit,
                       next);
        add_at(self, cur.index, cur.digit);
        cur.index += 1;
    }
    
#ifdef BIGINT_DEBUG
    print(self);
#endif
}

void big_int::add_at(BigInt& self, Index i, Digit d)
{
    if (!check_digit(d)) {
        DEBUG_PRINTFLN("d = %u not in range (0 <= d < %i)", d, DIGIT_BASE);
        return;
    }

    Pair cur{i, d};
    int  carry = 1; // Initialize to some nonzero so we can begin the loop.
    while (carry != 0) {
        int sum = read_at(self, cur.index) + cur.digit;
        if (sum >= DIGIT_BASE) {
            carry = sum / DIGIT_BASE;
            sum  %= DIGIT_BASE;
        } else {
            carry = 0;
        }
        DEBUG_PRINTFLN("digits[%zu] = %u + %i, sum = %u, carry = %u",
                       cur.index,
                       read_at(self, cur.index),
                       cur.digit,
                       sum,
                       carry);
        if (!write_at(self, cur.index, sum))
            break;
        cur.index += 1;
        cur.digit = carry;
    }

#ifdef BIGINT_DEBUG
    print(self);
#endif
}

// 2}}} ------------------------------------------------------------------------

// 1}}} ------------------------------------------------------------------------

// --- DIRECTIONAL MANIPULATION ------------------------------------------- {{{1

// --- LEFT --------------------------------------------------------------- {{{2

bool big_int::push_left(BigInt& self, Digit d)
{
    // TODO: Dynamically grow buffer
    // digits[capacity] is not readable or writable.
    if (self.length >= self.capacity) {
        DEBUG_PRINTFLN("Need resize, length = %zu, capacity %zu",
                       self.length,
                       self.capacity);
        return false;
    } else if (!check_digit(d)) {
        DEBUG_PRINTFLN("Cannot left-push digit %u", d);
        return false;
    }
    DEBUG_PRINTFLN("digits[%zu] = %u, length++", self.length, d);
    // Don't use `big_int::write_at()` since that does a bounds check.
    self.digits[self.length] = d;
    self.length += 1;
    return true;
}

big_int::Digit big_int::pop_left(BigInt& self)
{
    if (self.length == 0) {
        DEBUG_PRINTLN("Cannot left-pop digits[0], return 0");
        return 0;
    }
    // Don't use `read_at()` since that does a bounds check.
    Index i = self.length - 1;
    Digit d = self.digits[i];
    DEBUG_PRINTFLN("return digits[%zu], length--", i);
    self.length -= 1;
    return d;
}

bool big_int::shift_left1(BigInt& self)
{
    // TODO: Dynamically grow buffer
    if (self.length >= self.capacity) {
        DEBUG_PRINTFLN("Need resize, length = %zu, capacity %zu",
                       self.length,
                       self.capacity);
        return false;
    }
    DEBUG_PRINTFLN("lshift(length = %zu), length++", self.length);

    // {4,3,2,1}   -> {4,3,2,1,0}
    self.length += 1;

    // {4,3,2,1,0} -> {4,4,3,2,1}
    for (MutRevIt it = rbegin(self); it != rend(self); it++)
        *it = it[1]; // NOTE: Do not use it[-1]!

    // {4,4,3,2,1} -> {0,4,3,2,1}
    self.digits[0] = 0;
    return true;
}

// 2}}} ------------------------------------------------------------------------

// --- RIGHT -------------------------------------------------------------- {{{2

void big_int::push_right(BigInt& self, Digit d)
{
    if (check_digit(d)) {
        DEBUG_PRINTFLN("shift_left1(), digits[0] = %u", d);
        shift_left1(self); 
        self.digits[0] = d;
    } else {
        DEBUG_PRINTFLN("Cannot right-push digit %u", d);
    }
}

big_int::Digit big_int::pop_right(BigInt& self)
{
    // Nothing to pop?
    if (self.length == 0) {
        DEBUG_PRINTLN("Cannot right-pop digits[0], return 0");
        return 0;
    } else {
        DEBUG_PRINTLN("return digits[0], shift_right1()");
        Digit d = self.digits[0];
        shift_right1(self);
        return d;
    }
}

void big_int::shift_right1(BigInt& self)
{
    DEBUG_PRINTFLN("rshift(length = %zu), length--", self.length);
    
    // {4,3,2,1} -> {3,2,1,1}
    for (MutFwdIt it = begin(self); it != end(self); it++)
        *it = it[1];

    // Ensure buffer is clean after shifting.
    // {3,2,1,1} -> {3,2,1,0}
    self.digits[self.length - 1] = 0; 
    // {3,2,1,1} -> {3,2,1}
    self.length -= 1; 
    
}

// 2}}} ------------------------------------------------------------------------

// 1}}} ------------------------------------------------------------------------

// --- ITERATORS ---------------------------------------------------------- {{{1

big_int::MutFwdIt big_int::begin(BigInt& self)
{
    return &self.digits[0];
}

big_int::MutFwdIt big_int::end(BigInt& self)
{
    return self.digits + self.length;
}

big_int::ConstFwdIt big_int::begin(const BigInt& self)
{
    return &self.digits[0]; 
}

big_int::ConstFwdIt big_int::end(const BigInt& self)
{
    return self.digits + self.length;
}

big_int::MutRevIt big_int::rbegin(BigInt& self)
{
    return MutRevIt(end(self));
}

big_int::MutRevIt big_int::rend(BigInt& self)
{
    return MutRevIt(begin(self));
}

big_int::ConstRevIt big_int::rbegin(const BigInt& self)
{
    return ConstRevIt(end(self));
}

big_int::ConstRevIt big_int::rend(const BigInt& self)
{
    return ConstRevIt(begin(self));
}

// 1}}} ------------------------------------------------------------------------

// --- UTILITY ------------------------------------------------------------ {{{1

big_int::Digit big_int::read_at(const BigInt& self, Index i)
{
    // Conceptually, 00001234 == 1234, so 1234[7] should be 0 even if 1234
    // only allocated for 4 digits.
    if (check_index(self, i))
        return self.digits[i];
    else
        return 0;
}

bool big_int::write_at(BigInt& self, Index i, Digit d)
{
    if (check_index(self, i)) {
        DEBUG_PRINTFLN("digits[%zu] = %u", i, d);
        self.digits[i] = d;
        
        // We assume that all unused but valid regions are set to 0.
        if (i >= self.length)
            self.length = i + 1;
        return true;
    } else {
        // TODO resize buffer as needed and zero out newly extended regions
        DEBUG_PRINTFLN("invalid index %zu for digits[:%zu]", i, self.capacity);
        return false;
    }
}

void big_int::print(const BigInt& self)
{
    std::printf("digits[:%zu] = ", self.length);
    for (ConstRevIt it = rbegin(self); it != rend(self); it++)
        std::printf("%u", *it);
    std::printf(", length = %zu, capacity = %zu\n", self.length, self.capacity);
    
#ifdef BIGINT_DEBUG
    std::printf("\n");
#endif
}

bool big_int::check_digit(Digit d)
{
    return 0 <= d && d < DIGIT_BASE;
}

bool big_int::check_index(const BigInt& self, Index i)
{
    return 0 <= i && i < self.capacity;
}

// 1}}} ------------------------------------------------------------------------
