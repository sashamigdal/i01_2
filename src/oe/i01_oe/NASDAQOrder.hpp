#pragma once
#include <iosfwd>

#include <i01_core/MIC.hpp>
#include <i01_oe/Order.hpp>

namespace i01 { namespace OE {

namespace NASDAQ { class OUCH42Session; }

class NASDAQOrder : public Order
{
public:
    // OUCH42:
  enum class Display : std::uint8_t {
      UNKNOWN                                     = '\0'
    , ATTRIBUTABLE_PRICE_TO_DISPLAY               = 'A'
    , ANONYMOUS_PRICE_TO_COMPLY                   = 'Y'
    , NON_DISPLAY                                 = 'N'
    , POST_ONLY                                   = 'P'
    , IMBALANCE_ONLY                              = 'I'
    , MIDPOINT_PEG                                = 'M'
    , MIDPOINT_PEG_POST_ONLY                      = 'W'
    , POST_ONLY_AND_ATTRIBUTABLE_PRICE_TO_DISPLAY = 'L'
    , RETAIL_ORDER_TYPE_1                         = 'O'
    , RETAIL_ORDER_TYPE_2                         = 'T'
    , RETAIL_PRICE_IMPROVEMENT_ORDER              = 'Q'
  };
    friend std::ostream& operator<<(std::ostream& os, const Display& d);

    // OUCH42:
  enum class Capacity : std::uint8_t {
      AGENCY      = 'A'
    , PRINCIPAL   = 'P'
    , RISKLESS    = 'R'
    , OTHER       = 'O'
  };
    friend std::ostream& operator<<(std::ostream& os, const Capacity& c);

  enum class LiquidityFlag : std::uint8_t {
      // Don't forget to update OE::Types::FillFeeCode if you change this enum!
      UNKNOWN                                           = '\0'
    , ADDED                                             = 'A'
    , REMOVED                                           = 'R'
    , OPENING_CROSS_BILLABLE                            = 'O'
    , OPENING_CROSS_NON_BILLABLE                        = 'M'
    , CLOSING_CROSS_BILLABLE                            = 'C'
    , CLOSING_CROSS_NON_BILLABLE                        = 'L'
    , HALT_OR_IPO_CROSS_BILLABLE                        = 'H'
    , HALT_OR_IPO_CROSS_NON_BILLABLE                    = 'K'
    , NON_DISPLAYED_ADDING_LIQUIDITY                    = 'J'
    , ADDED_POST_ONLY                                   = 'W' /* N/A */
    , REMOVED_LIQUIDITY_AT_A_MIDPOINT                   = 'm'
    , ADDED_LIQUIDITY_VIA_MIDPOINT_ORDER                = 'k'
    , SUPPLEMENTAL_ORDER_EXECUTION                      = '0' /* zero */
    , DISPLAYED_ADDING_IMPROVES_NBBO                    = '7'
    , DISPLAYED_ADDING_SETS_QBBO_JOINS_NBBO             = '8'
    , RETAIL_DESIGNATED_EXECUTION_REMOVED_LIQUIDITY     = 'd' /* N/A */
    , RETAIL_DESIGNATED_ADDED_DISPLAYED_LIQUIDITY       = 'e'
    , RETAIL_DESIGNATED_ADDED_NONDISPLAYED_LIQUIDITY    = 'f' /* N/A */
    , RETAIL_PRICE_IMPROVING_PROVIDES_LIQUIDITY         = 'j'
    , RETAIL_REMOVES_RPI_LIQUIDITY                      = 'r'
    , RETAIL_REMOVES_OTHER_PRICE_IMPROVING_NONDISPLAYED = 't'
    , ADDED_DISPLAYED_LIQUIDITY_IN_SELECT_SYMBOL        = '4'
    , ADDED_NON_DISPLAYED_LIQUIDITY_IN_SELECT_SYMBOL    = '5'
    , LIQUIDITY_REMOVING_ORDER_IN_SELECT_SYMBOL         = '6'
    , ADDED_NON_DISPLAYED_MIDPOINT_LIQUIDITY_IN_SELECT  = 'g'
  };

  NASDAQOrder(Instrument* const iinstrument,
              const Price iprice,
              const Size isize,
              const Side iside,
              const TimeInForce itif,
              const OrderType itype,
              OrderListener* ilistener,
              const UserData iuserdata = nullptr,
              const Display idisplay = Display::UNKNOWN,
              const Capacity icapacity = Capacity::AGENCY,
              const Size iminsize = 0)
   : Order(iinstrument, iprice, isize, iside, itif, itype, ilistener, iuserdata)
   , m_nasdaq_display(idisplay)
   , m_nasdaq_capacity(icapacity)
   , m_nasdaq_min_size(iminsize)
   , m_nasdaq_tif_timed(0)
   , m_iso_eligible(false)
   , m_liquidity_flag(LiquidityFlag::UNKNOWN)
   , m_match_number(0)
  {}

  bool validate() const final { return true; }
  const i01::core::MIC market() const override final { return i01::core::MICEnum::XNAS; }
  NASDAQOrder* clone() const override final { return new NASDAQOrder(*this); }

  Display nasdaq_display() const { return m_nasdaq_display; }
  void nasdaq_display(const Display& d) { m_nasdaq_display = d; }

  Capacity nasdaq_capacity() const { return m_nasdaq_capacity; }
  void nasdaq_capacity(const Capacity& c) { m_nasdaq_capacity = c; }

  Size nasdaq_min_size() const { return m_nasdaq_min_size; }
protected:
  Display m_nasdaq_display;
  Capacity m_nasdaq_capacity;
  Size m_nasdaq_min_size;
  std::uint32_t m_nasdaq_tif_timed; // placeholder
  bool m_iso_eligible; // placeholder
  char m_bbo_weight_indicator; // placeholder
  char m_cancel_reason; // placeholder
  LiquidityFlag m_liquidity_flag; // placeholder
  std::uint64_t m_match_number; // placeholder

  friend i01::OE::NASDAQ::OUCH42Session;
};

}}
