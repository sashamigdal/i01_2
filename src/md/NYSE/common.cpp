#include <i01_md/NYSE/common.hpp>

std::uint64_t i01::MD::NYSE::nyse_price_to_i01(std::uint32_t numerator, std::uint8_t nyse_scale)
{
    // i01 has scale of 4 ...
    int scale = 4 - nyse_scale;

    if (scale > 0) {
        for (int i = 0; i < scale; i++) {
            numerator *= 10;
        }
    } else {
        for (int i = 0 ; i < -scale; i++) {
            numerator /= 10;
        }
    }
    return numerator;
}
