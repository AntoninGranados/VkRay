#include "math_utils.hpp"

size_t nextPowerOfTwo(size_t count, size_t minimum) {
    size_t cap = minimum;
    while (cap < count) cap <<= 1;
    return cap;
}
