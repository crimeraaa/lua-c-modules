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
    using FwdIt = const Digit*;
    using RevIt = std::reverse_iterator<const Digit*>;

    enum class Base {
        Bin = 2,
        Oct = 8,
        Dec = 10,
        Hex = 16,
    };

    static constexpr Size NUM_DIGITS = 0x100;
    static constexpr Base NUM_BASE   = Base::Dec;
    
    
    BigInt(Base b = Base::Dec)
    : m_digits{0}
    , m_length{0}
    , m_base{b}
    {
        for (Size i = 0; i < capacity(); i++)
            m_digits[i] = 0;
    }
    
    BigInt(std::string_view s, Base b = Base::Dec)
    : BigInt(b)
    {
        // We use the reverse iterator so we can add the characters from right
        // to left. We will lay out our integer in little-endian fashion.
        for (auto it = s.rbegin(); it != s.rend(); it++) {
            char ch = *it;
            if (std::isspace(ch) || ch == '_')
                continue;
            push_left(convert_char(ch));
        }
    }
   
    FwdIt begin() const
    {
        return &m_digits[0];
    }
    
    FwdIt end() const
    {
        // &digits[length], where length >= NUM_DIGITS, technically UB!
        return m_digits + length();
    }
    
    RevIt rbegin() const
    {
        return RevIt(end());
    }
    
    RevIt rend() const
    {
        return RevIt(begin());
    }
    
    constexpr int base() const
    {
        return static_cast<int>(NUM_BASE);
    }
    
    // Number of digits we are actively using.
    Size length() const
    {
        return m_length;
    }
    
    // Number of digits we can hold in total.
    constexpr Size capacity() const
    {
        return NUM_DIGITS;
    }

    // --- BUFFER MANIPULATION -------------------------------------------- {{{1
    
    // Conceptually, any digit we aren't currently aware of is 0.
    Digit read_at(Size i) const
    {
        if (check_index(i))
            return m_digits[i];
        else
            return 0;
    }
    
    // let d = 5:
    // User-facing:     1234      -> 51234
    // Implementation:  {4,3,2,1} -> {4,3,2,1,5}
    void push_left(Digit d)
    {
        if (write_at(length(), d))
            m_length += 1;
    }
    
    // User-facing:     1234      -> 234
    // Implementation:  {4,3,2,1} -> {4,3,2}
    Digit pop_left()
    {
        if (length() > 0) {
            Digit d = read_at(length() - 1);
            m_length -= 1;
            return d;
        }
        return 0;
    }

    // User-facing:     1234      -> 12340
    // Implementation:  {4,3,2,1} -> {0,4,3,2,1}
    void shift_left1()
    {
        // 1234 -> 01234, {4,3,2,1} -> {4,3,2,1,0}
        push_left(0);
    
        // 01234 -> 12340, {4,3,2,1,0} -> {0,4,3,2,1}
        for (Size i = length() - 1; /* (none) */; i--) {
            write_at(i, read_at(i - 1));
            // Need this here due to how unsigned ints overflow.
            if (i == 0)
                break;
        }
    }
    
    // let d = 0:
    // User-facing:     1234      -> 12340
    // Implementation:  {4,3,2,1} -> {0,4,3,2,1}
    void push_right(Digit d)
    {
        shift_left1();
        write_at(0, d);
    }
    
    // TODO: Implicitly extend as needed?
    bool write_at(Size i, Digit d)
    {
        if (!check_index(i)) {
            std::fprintf(stderr, "Cannot write to index %zu\n", i);
            return false;
        } else if (!check_digit(d)) {
            std::fprintf(stderr, "Invalid digit %u", d);
            return false;
        } else {
            m_digits[i] = d;
            return true;
        }
    }

    // 1}}} --------------------------------------------------------------------
    
    void add_at(Size i, Digit d)
    {
        Digit sum   = read_at(i) + d;
        Digit carry = 0;
        if (sum >= base()) {
            carry = 1;
            sum  -= base();
            // This recursion may get messy!
            if (check_index(i + 1))
                add_at(i + 1, carry);
        }
        write_at(i, sum);
    }
    
    void print() const
    {
        Size len = length();
        printf("base = %i, length = %zu, digits[0:%zu] = ", base(), len, len);
        for (RevIt it = rbegin(); it != rend(); it++)
            printf("%u", *it);
        printf("\n");
    }

private:
    const BigInt* const_this()
    {
        return const_cast<const BigInt*>(this);
    }
    
    Digit convert_char(char ch) const
    {
        if constexpr(NUM_BASE == Base::Bin) {
            if ('0' <= ch && ch <= '1')
                return ch - '0';
        }
        
        if constexpr(NUM_BASE == Base::Oct) {
            if ('0' <= ch && ch <= '7')
                return ch - '0';
        }
        
        if constexpr(NUM_BASE == Base::Dec || NUM_BASE == Base::Hex) {
            if ('0' <= ch && ch <= '9')
                return ch - '0';
            
            if constexpr(NUM_BASE == Base::Hex) {
                if ('a' <= ch && ch <= 'f')
                    return ch - 'a' + 0xa;
                if ('A' <= ch && ch <= 'F')
                    return ch - 'A' + 0xa;
            }
        }
        return 0;
    }
    
    bool check_digit(Digit d) const
    {
        return 0 <= d && d < base();
    }

    bool check_index(Size i) const
    {
        return 0 <= i && i < capacity();
    }

    Digit m_digits[NUM_DIGITS];
    Size  m_length; // #active digits.
    Base  m_base;
};

int main()
{
    BigInt a("1234");
    a.push_left(0);
    a.push_right(5);
    a.print();
    
    BigInt b("1234");
    b.pop_left();
    b.print();
    
    BigInt c("1234");
    c.add_at(0, 8);
    c.print();
    return 0;
}
