#pragma once

#include <cstdint>
#include <array>

namespace i01 { namespace MD { namespace TSE { inline namespace v1 { namespace Types {

namespace {
    static inline constexpr std::uint16_t charstoshort(char x, char y) { return ((std::uint16_t)x << 8) + y; }
}

enum class TagIDEnum : std::uint16_t {
    UNKNOWN            = 0
  , SPACE              = charstoshort(' ',' ')
  , CONTROL            = charstoshort('L','C')
  , UPDATE_NO          = charstoshort('N','O')
  , STATUS_INFORMATION = charstoshort('S','T')
  // Full:
  , CURRENT_PRICE      = charstoshort('1','P')
  , TRADING_VOLUME     = charstoshort('V','L')
  , TURNOVER           = charstoshort('V','A')
  , ASK_QUOTE          = charstoshort('Q','S')
  , BID_QUOTE          = charstoshort('Q','B')
  , CLOSING_SELL       = charstoshort('S','C')
  , CLOSING_BUY        = charstoshort('B','C')
  // Standard:
    // TODO
  // Basic Issue:
  , ISSUE_INFORMATION  = charstoshort('I','I')
  , BASE_PRICE         = charstoshort('B','P')
  , MCAST_GROUP_INFO   = charstoshort('M','G')
  // High-Speed Index:
  , HSI_SERIAL_NUMBER  = charstoshort('S','N')
  , HSI_STOCK_INDEX    = charstoshort('S','I')
  , HSI_BEST_ASK       = charstoshort('A','I')
  , HSI_BEST_BID       = charstoshort('B','I')
  // TCP:
  , TCP_CONTROL        = charstoshort('T','C')
};

union TagID {
    std::uint8_t                raw[2];
    std::array<std::uint8_t, 2> arr;
    std::uint16_t               u16;
    TagIDEnum                   tag;
};

} } } } }
