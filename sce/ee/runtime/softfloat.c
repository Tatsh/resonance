// The double comparisons the original runtime made. Its soft-float library sent every comparison
// through one three-way compare (0x004b75b8, with the part compare at 0x004b74a0), and the
// compare reports an unordered pair as greater. A > or >= test with a NaN operand therefore
// succeeded. The compiler's __gtdf2 and __gedf2 report the pair as less, and the definitions
// below replace them. The other comparisons already give the original result.

#include <stdint.h>
#include <string.h>

typedef int cmp_type __attribute__((mode(__libgcc_cmp_return__)));

enum {
    kSignShift = 63,
};

static const uint64_t kMagnitudeMask = 0x7fffffffffffffffULL;
static const uint64_t kInfinityBits = 0x7ff0000000000000ULL;

static cmp_type compare_double(double a, double b) {
    uint64_t a_bits;
    uint64_t b_bits;
    memcpy(&a_bits, &a, sizeof(a_bits));
    memcpy(&b_bits, &b, sizeof(b_bits));

    const uint64_t a_magnitude = a_bits & kMagnitudeMask;
    const uint64_t b_magnitude = b_bits & kMagnitudeMask;
    if ((a_magnitude > kInfinityBits) || (b_magnitude > kInfinityBits)) {
        return 1;
    }
    if ((a_magnitude == 0) && (b_magnitude == 0)) {
        return 0;
    }

    const int a_negative = (int)(a_bits >> kSignShift);
    const int b_negative = (int)(b_bits >> kSignShift);
    if (a_negative != b_negative) {
        return a_negative ? -1 : 1;
    }
    if (a_magnitude == b_magnitude) {
        return 0;
    }
    const int magnitude_less = a_magnitude < b_magnitude;
    return (magnitude_less != a_negative) ? -1 : 1;
}

// The original three-way compare with its operands in source order. The compiler moves a constant
// to the right of a comparison before any later pass runs, and the original result for an unordered
// pair depends on which side each operand was written on. Source that compares a constant against
// a double therefore calls freq_compare_double() directly.
int freq_compare_double(double a, double b) {
    return (int)compare_double(a, b);
}

cmp_type __gtdf2(double a, double b) {
    return compare_double(a, b);
}

cmp_type __gedf2(double a, double b) {
    return compare_double(a, b);
}
