#include <cctype>
#include <cstdio>
#include <cstdint>
#include <string_view>
#include <iterator>
#include "common.hpp"

class BigInt {
public:
    using Digit = std::uint8_t;
    using Size  = std::size_t;
    using Iter  = const Digit*;

    static constexpr size_t NUM_DIGITS = 0x10;
    static constexpr int    NUM_BASE   = 10;
    
    BigInt()
    : m_digits{0}
    , m_length{0}
    {
        for (size_t i = 0; i < capacity(); i++)
            m_digits[i] = 0;
    }
    
    BigInt(std::string_view s)
    : BigInt()
    {
        // We use the reverse iterator so we can add the characters from right
        // to left. Our buffer is also written from right to left.
        for (auto it = s.rbegin(); it != s.rend(); it++)
            push_front(std::isdigit(*it) ? *it - '0' : 0);
    }
    
    const Iter begin() const
    {
        // NOTE: Leftmost elements may be just zeroes, hence we do it like this.
        return &m_digits[to_index(length() - 1)];
    }
    
    const Iter end() const
    {
        // &digits[length], where length >= NUM_DIGITS, technically UB!
        return m_digits + capacity();
    }
    
    // Number of digits we are actively using.
    Size length() const
    {
        return m_length;
    }
    
    // Number of digits we can hold in total.
    Size capacity() const
    {
        return NUM_DIGITS;
    }

    // --- BUFFER MANIPULATION -------------------------------------------- {{{1
    
    Digit read_at(size_t i) const
    {
        if (in_range(i))
            return m_digits[to_index(i)];
        else
            return 0;
    }
    
    void push_front(Digit d)
    {
        if (write_at(length(), d))
            m_length += 1;
    }
    
    Digit pop_front()
    {
        if (length() > 0) {
            Digit d = read_at(length() - 1);
            m_length -= 1;
            return d;
        }
        return 0;
    }
    
    bool write_at(size_t i, Digit d)
    {
        if (!in_range(i)) {
            std::fprintf(stderr, "Cannot write to index %zu\n", i);
            return false;
        } else if (d >= 10) {
            std::fprintf(stderr, "Invalid digit %u", d);
            return false;
        } else {
            m_digits[to_index(i)] = d;
            return true;
        }
    }

    // 1}}} --------------------------------------------------------------------
    
    void print() const
    {
        Size len = length();
        Size cap = capacity();
        printf("length = %zu, digits[%zu:%zu] = ", len, to_index(len - 1), cap - 1);
        for (Iter it = begin(); it != end(); it++)
            printf("%u", *it);
        printf("\n");
    }

private:
    // Allow our indexes to be written from right to left.
    // E.g. if capacity() = 8, user-facing index 0 is actually 7 internally.
    Size to_index(Size i) const
    {
        return capacity() - 1 - i;
    }
    
    bool in_range(size_t i) const
    {
        return 0 <= i && i < capacity();
    }

    Digit m_digits[NUM_DIGITS];
    Size  m_length; // #active digits.
};

int main()
{
    BigInt("1").print();
    BigInt("12").print();
    BigInt("1234").print();
    BigInt("12345678").print();
    // BigInt("1234567890").print();

    BigInt a = BigInt("123456789");
    a.push_front(0);
    a.print();
    return 0;
}
