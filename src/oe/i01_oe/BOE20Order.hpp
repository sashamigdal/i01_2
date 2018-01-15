#pragma once

#include <iosfwd>

#include <i01_core/Alphanumeric.hpp>

#include <i01_oe/Order.hpp>

namespace i01 { namespace OE {

class BOE20Order : public Order {
public:
    // define some BOE20 types that are visible in the order, but will
    // also be used by the session.

    template<int WIDTH> using BOEAlphanumeric = core::Alphanumeric<WIDTH, true, false, '\0'>;
    template<int WIDTH> using FixedWidthText = BOEAlphanumeric<WIDTH>;

    enum class DisplayIndicator : std::uint8_t {
        UNKNOWN                         = '\0'
    , DEFAULT                           = 'V'
    , PRICE_ADJUST                      = 'P'
    , MULTIPLE_PRICE_ADJUST             = 'm'
    , CANCEL_BACK_IF_NEEDS_ADJUSTMENT   = 'R'
    , HIDDEN                            = 'r'
    , DISPLAY_PRICE_SLIDING             = 'S'
    , DISPLAY_PRICE_SLIDING_NO_CROSS    = 'L'
    , MULTIPLE_DISPLAY_PRICE_SLIDING    = 'M'
    , HIDE_NOT_SLIDE_DEPRECATED         = 'H'
    , VISIBLE                           = 'v'
    , INVISIBLE                         = 'I'
    , NO_RESCRAPE_AT_LIMIT              = 'N'
    };
    friend std::ostream& operator<<(std::ostream& os, const DisplayIndicator& d);

    enum class ExecInst : std::uint8_t {
        UNKNOWN                         = '\0'
    , DEFAULT                           = ' '
    , INTERMARKET_SWEEP                 = 'f'
    , MARKET_PEG                        = 'P'
    , MARKET_MAKER_PEG                  = 'Q'
    , PRIMARY_PEG                       = 'R'
    , SUPPLEMENTAL_PEG                  = 'U'
    , LISTING_MARKET_OPENING            = 'o'
    , LISTING_MARKET_CLOSING            = 'c'
    , BOTH_LISTING_MARKET_OPEN_N_CLOSE  = 'a'
    , MIDPOINT_DISCRETIONARY            = 'd'
    , MIDPOINT_PEG_TO_NBBO_MID          = 'M'
    , MIDPOINT_PEG_TO_NBBO_MID_NO_LOCK  = 'm'
    , ALTERNATIVE_MIDPOINT              = 'L'
    , LATE                              = 'r'
    , MIDPOINT_MATCH_DEPRECATED         = 'I'
    };
    friend std::ostream& operator<<(std::ostream& os, const ExecInst& e);

    enum class RoutingInstFirstChar : std::uint8_t {
        UNKNOWN                         = '\0'
    , BOOK_ONLY                         = 'B'
    , POST_ONLY                         = 'P'
    , POST_ONLY_AT_LIMIT                = 'Q'
    , ROUTABLE                          = 'R'
    , SUPER_AGGRESSIVE                  = 'S'
    , AGGRESSIVE                        = 'X'
    , SUPER_AGGRESSIVE_WHEN_ODD_LOT     = 'K'
    , POST_TO_AWAY                      = 'A'
    };
    friend std::ostream& operator<<(std::ostream& os, const RoutingInstFirstChar& ri);

    enum class RoutingInstSecondChar : std::uint8_t {
        UNKNOWN                         = '\0'
    , ELIGIBLE_TO_ROUTE_TO_DRT_CLC    = 'D'
    , ROUTE_TO_DISPLAYED_MKTS_ONLY      = 'L'
    };
    friend std::ostream& operator<<(std::ostream& os, const RoutingInstSecondChar& ri);

    using RoutingInstText = FixedWidthText<4>;
    union RoutingInst {
        std::uint32_t                           u32;
        RoutingInstText                         text;
        struct RoutingInstLayout {
            RoutingInstFirstChar                first;
            RoutingInstSecondChar               second;
            std::uint8_t                        unused[2];
        } __attribute__ ((packed))              layout;
        void set(const RoutingInstFirstChar w, const RoutingInstSecondChar = RoutingInstSecondChar::UNKNOWN);
    } __attribute__((packed));
    I01_ASSERT_SIZE(RoutingInst, 4);
    friend std::ostream& operator<<(std::ostream& os, const RoutingInst& ri);

    // inline constexpr std::uint32_t RoutingInstValue(const RoutingInstFirstChar w = RoutingInstFirstChar::UNKNOWN,
    //                                                 const RoutingInstSecondChar x = RoutingInstSecondChar::UNKNOWN) const {
    //     return (((std::uint32_t)w << 0) + ((std::uint32_t)x << 8));
    // }

public:
    BOE20Order(Instrument* const iinst,
               const Price iprice,
               const Size isize,
               const Side iside,
               const TimeInForce itif,
               const OrderType itype,
               OrderListener* ilistener,
               const UserData iuserdata = nullptr);

    DisplayIndicator boe_display_indicator() const { return m_display_indicator; }
    void boe_display_indicator(const DisplayIndicator& di) { m_display_indicator = di; }
    bool boe_display_indicator(const std::string& value);

    ExecInst boe_exec_inst() const {return m_exec_inst; }
    void boe_exec_inst(const ExecInst& ei) { m_exec_inst = ei; }

    RoutingInst boe_routing_inst() const { return m_routing_inst; }
    void boe_routing_inst(const RoutingInst& ri) { m_routing_inst = ri; }
    bool boe_routing_inst(const std::string& v);

    RoutingInstFirstChar boe_routing_inst_first_char() const { return m_routing_inst.layout.first; }
    void boe_routing_inst_first_char(const RoutingInstFirstChar& rif) { m_routing_inst.layout.first = rif; }

    void set_post_only();

    bool set_attribute(const std::string& name, const std::string& value);

    friend std::ostream& operator<<(std::ostream& os, const BOE20Order& o);

protected:
    DisplayIndicator m_display_indicator;
    ExecInst m_exec_inst;
    RoutingInst m_routing_inst;

};

}}
