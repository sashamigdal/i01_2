#pragma once

#include <cstdint>

#include <i01_core/macro.hpp>

namespace i01 { namespace OE { namespace EDGE { namespace XPRS130 { namespace Types {

    /* SESSION API TYPES */

    typedef std::uint16_t PackageLength;

    enum class PackageType : std::uint8_t {
        LOGIN_REQUEST           = 'L'
      , LOGIN_ACCEPTED          = 'A'
      , LOGIN_REJECTED          = 'J'
      , SEQUENCED_DATA          = 'S'
      , UNSEQUENCED_DATA        = 'U'
      , SERVER_HEARTBEAT        = 'H'
      , CLIENT_HEARTBEAT        = 'R'
      , LOGOUT_REQUEST          = 'O'
      , DEBUG                   = '+'
      , END_OF_SESSION          = 'Z'
    };

    typedef std::uint8_t Username[6];
    typedef std::uint8_t Password[10];
    typedef std::uint8_t Session[10];
    typedef std::uint8_t SequenceNumber[20];

    enum class RejectReasonCode : std::uint8_t {
        NOT_AUTHORIZED            = 'A'
      , SESSION_NOT_AVAILABLE     = 'S'
    };

    /* ORDER API TYPES */

    enum class MessageType : std::uint8_t {
        ENTER_ORDER_SHORT_FORMAT          = 'O'
      , ENTER_ORDER_EXTENDED_FORMAT       = 'N'
      , ACCEPTED_MESSAGE_SHORT_FORMAT     = 'A'
      , ACCEPTED_MESSAGE_EXTENDED_FORMAT  = 'P'
      , EXECUTED_ORDER                    = 'E'
      , REJECTED_MESSAGE                  = 'J'
      , EXTENDED_REJECTED_MESSAGE         = 'L'
      , CANCEL_ORDER                      = 'X'
      , CANCELED_MESSAGE                  = 'C'
      , CANCEL_PENDING                    = 'I'
      , REPLACE_ORDER                     = 'U'
      , REPLACED_MESSAGE                  = 'R'
      , PENDING_REPLACE                   = 'D'
      , SYSTEM_EVENT                      = 'S'
      , BROKEN_TRADE                      = 'B'
      , PRICE_CORRECTION                  = 'K'
      , ANTI_INTERNALIZATION_MODIFIER     = 'F'
      , AI_ADDITIONAL_INFO_MESSAGE        = 'G'
    };

    typedef std::uint8_t OrderToken[14];
    
    enum class BuySellIndicator : std::uint8_t {
        BUY                   = 'B'
      , SELL                  = 'S'
      , SELL_SHORT            = 'T'
      , SELL_SHORT_EXEMPT     = 'E'
      , SELL_SHORT_SS_EXEMPT  = 'X'
    };

    typedef std::uint32_t Quantity;
    typedef std::uint8_t Symbol[6];
    typedef std::uint32_t Price;

    enum class TimeInForce : std::uint8_t {
        IMMEDIATE_OR_CANCEL   = 0
      , DAY                   = 1
      , FILL_OR_KILL          = 2
    };

    enum class Display : std::uint8_t {
        DISPLAYED                   = 'Y'
      , HIDDEN                      = 'N'
      , DISPLAYED_WITH_ATTRIBUTION  = 'A'
    };

    enum class SpecialOrderType : std::uint8_t {
        MIDPOINT_MATCH_EDGX_MIDPOINT_PEG_EDGA   = 'M'
      , MIDPOINT_DISCRETIONARY_ORDER_EDGA_ONLY  = 'D' /* requires Display=Y */
      , ROUTE_PEG_ORDER                         = 'U'
      , NBBO_OFFSET_PEG_MARKET_MAKER_PEG        = 'N'
      , PRIMARY_PEG                             = 'X' /* extended only */
      , MARKET_PEG                              = 'Y' /* extended only */
      , HIDE_NOT_SLIDE                          = 'S'
      , PRICE_ADJUST                            = 'P'
      , SINGLE_RE_PRICE                         = 'R'
      , CANCEL_BACK                             = 'C'
    };

    enum class ExtendedHoursEligible : std::uint8_t {
        REGULAR_SESSION_ONLY                = 'R'
      , PRE_MARKET_AND_REGULAR_SESSION      = 'P'
      , REGULAR_SESSION_AND_POST_MARKET     = 'A'
      , ALL_SESSIONS                        = 'B'
    };

    enum class Capacity : std::uint8_t {
        AGENCY              = 'A'
      , PRINCIPAL           = 'P'
      , RISKLESS_PRINCIPAL  = 'R'
    };

    enum class RouteOutEligibility : std::uint8_t {
        ROUTABLE_ROUT_STRATEGY_ONLY                     = 'Y'
      , BOOK_ONLY                                       = 'N'
      , POST_ONLY                                       = 'P'
      , SUPER_AGGRESSIVE_CROSS_OR_LOCK                  = 'S'
      , AGGRESSIVE_CROSS_ONLY                           = 'X'
      , POST_TO_EDGA                                    = 'a'
      , POST_TO_EDGX                                    = 'b'
      , POST_TO_NYSE_ARCA                               = 'c'
      , POST_TO_NYSE                                    = 'd'
      , POST_TO_NASDAQ                                  = 'e'
      , POST_TO_NASDAQ_BX                               = 'f'
      , POST_TO_NASDAQ_PSX                              = 'g'
      , POST_TO_BATS_BYX                                = 'h'
      , POST_TO_BATS_BZX                                = 'i'
      , POST_TO_LAVAFLOW                                = 'j'
      , POST_TO_CBSX                                    = 'k'
      , POST_TO_AMEX                                    = 'l'
      , POST_TO_NSX                                     = 'n'
      , NOT_APPLICABLE                                  = ' '
    };

    enum class ISOEligibility : std::uint8_t {
        YES = 'Y'
      , NO  = 'N'
    };

    enum class RoutingDeliveryMethod : std::uint8_t {
        ROUTE_TO_IMPROVE  = 'I'
      , ROUTE_TO_FILL     = 'F'
      , ROUTE_TO_COMPLY   = 'C'
    };

    enum class RouteStrategy : std::uint16_t {
        ROUT  = 1   //< Book + Low Cost/CLC + Street (Default)
      , ROUD  = 2   //< Book + Select CLC
      , ROUE  = 3   //< Book + Low Cost/Select CLC + Street
      , ROUX  = 4   //< Book + Street
      , ROUZ  = 5   //< Book + Low Cost/CLC
      , ROUQ  = 6   //< Book + Select Fast CLCs
      , RDOT  = 7   //< Book + Low Cost/CLC + DOT
      , RDOX  = 8   //< Book + DOT
      , ROPA  = 9   //< Book + IOC ARCA
      , ROBA  = 10  //< Book + IOC BATS
      , ROBX  = 11  //< Book + IOC Nasdaq BX
      , INET  = 12  //< Book + Nasdaq
      , IOCT  = 13  //< EDGA/X Book + CLC + Other EDGA/X Book
      , IOCX  = 14  //< EDGA/X Book + Other EDGA/X Book
      , ISAM  = 15  //< Directed IOC ISO routed to AMEX
      , ISPA  = 16  //< Directed IOC ISO routed to ARCA
      , ISBA  = 17  //< Directed IOC ISO routed to BATS
      , ISBX  = 18  //< Directed IOC ISO routed to Nasdaq BX
      , ISCB  = 19  //< Directed IOC ISO routed to CBSX
      , ISCX  = 20  //< Directed IOC ISO routed to CHSX
      , ISCN  = 21  //< Directed IOC ISO routed to NSX
      , ISGA  = 22  //< Directed IOC ISO routed to EDGA
      , ISGX  = 23  //< Directed IOC ISO routed to EDGX
      , ISLF  = 24  //< Directed IOC ISO routed to LavaFlow
      , ISNQ  = 25  //< Directed IOC ISO routed to Nasdaq
      , ISNY  = 26  //< Directed IOC ISO routed to NYSE
      , ISPX  = 27  //< Directed IOC ISO routed to PHLX
      , ROUC  = 29  //< Book + CLC + Nasdaq BX + DOT + Posted to EDGX
      , ROLF  = 30  //< Book + LavaFlow
      , ISBY  = 31  //< Directed IOC ISO routed to BYX
      , SWPA  = 32  //< IOC ISO Sweep of All Protected Mkts
      , SWPB  = 33  //< IOC ISO Sweep of all Protected Mkts
      , IOCM  = 34  //< EDGA Book + IOC MPM to EDGX
      , ICMT  = 35  //< EDGA Book + CLC + IOC MPM to EDGX
      , ROOC  = 36  //< Listing Mkt Open + Book + Low Cost/CLC + Street + Listing Mkt Close
      , ROBY  = 37  //< Book + IOC BYX
      , ROBB  = 38  //< EDGA Book + IOC Nasdaq BX + IOC BYX
      , ROCO  = 39  //< EDGA Book + IOC Nasdaq BX + IOC BYX + CLC + EDGX MPM
      , SWPC  = 40  //< IOC ISO Sweep of All Protected Mkts and Post Remainder
      , RMPT  = 42  //< Book + EDGX MPM + CLC Midpoint + Street Midpoint
    };

    typedef std::int8_t PegDifference;
    typedef std::int8_t DiscretionaryOffset;
    typedef std::uint64_t Timestamp;
    typedef std::uint64_t OrderReferenceNumber;

    inline
    constexpr std::uint16_t LiquidityFlagValue (
                                     const std::uint8_t w = ' ',
                                     const std::uint8_t x = ' ')
    {
        return (std::uint16_t)
            (((std::uint16_t)w << 0) + ((std::uint16_t)x << 8));
    }

    typedef std::uint8_t LiquidityFlagRaw[5];
    enum class LiquidityFlag16 : std::uint16_t {
        ROUTED_TO_NASDAQ_ADDS_LIQUIDITY               = LiquidityFlagValue('A')
      , ADD_LIQUIDITY_TO_OUR_BOOK_TAPE_B              = LiquidityFlagValue('B')
      , ROUTED_TO_NASDAQ_BX_REMOVES_LIQUIDITY         = LiquidityFlagValue('C')
      , ROUTED_OR_REROUTED_TO_NYSE_REMOVES_LIQUIDITY  = LiquidityFlagValue('D')
      , CUSTOMER_INTERNALIZATION_ADDED_LIQUIDITY = LiquidityFlagValue('E', 'A')
      , CUSTOMER_INTERNALIZATION_REMOVED_LIQUIDITY=LiquidityFlagValue('E', 'R')
      , ROUTED_TO_NYSE_ADDS_LIQUIDITY                 = LiquidityFlagValue('F')
      , ROUTED_TO_ARCA_REMOVES_LIQUIDITY              = LiquidityFlagValue('G')
      , HIDDEN_ORDER_ADDS_LIQUIDITY               =LiquidityFlagValue('H', 'A')
      , HIDDEN_ORDER_REMOVES_LIQUIDITY            =LiquidityFlagValue('H', 'B')
      , ROUTED_TO_EDGA_OR_TO_EDGX                     = LiquidityFlagValue('I')
      , ROUTED_TO_NASDAQ_REMOVES_LIQUIDITY            = LiquidityFlagValue('J')
      , ROUTED_TO_BATS_USING_ROBA_OR_PSX_USING_ROUC   = LiquidityFlagValue('K')
      , ROUTED_TO_NASDAQ_USING_INET_REMOVES_LIQUIDITY = LiquidityFlagValue('L')
      , ADD_LIQUIDITY_ON_LAVAFLOW                     = LiquidityFlagValue('M')
      , REMOVE_LIQUIDITY_FROM_OUR_BOOK_TAPE_C         = LiquidityFlagValue('N')
      , LISTING_MARKET_OPENING_CROSS                  = LiquidityFlagValue('O')
      , ADD_LIQUIDITY_ON_EDGX_VIA_EDGA_ROUC_ORDER     = LiquidityFlagValue('P')
      , ROUTED_USING_ROUQ_OR_ROUC                     = LiquidityFlagValue('Q')
      , REROUTED_BY_EXCHANGE                          = LiquidityFlagValue('R')
      , DIRECTED_ISO_ORDER                            = LiquidityFlagValue('S')
      , ROUTED_USING_ROUD_OR_ROUE                     = LiquidityFlagValue('T')
      , REMOVE_LIQUIDITY_FROM_LAVAFLOW                = LiquidityFlagValue('U')
      , ADD_LIQUIDITY_TO_OUR_BOOK_TAPE_A              = LiquidityFlagValue('V')
      , REMOVE_LIQUIDITY_FROM_OUR_BOOK_TAPE_A         = LiquidityFlagValue('W')
      , ROUTED                                        = LiquidityFlagValue('X')
      , ADD_LIQUIDITY_TO_OUR_BOOK_TAPE_C              = LiquidityFlagValue('Y')
      , ROUTED_USING_ROUZ_OR_EXECUTED_IN_CLC_FROM_ICMT= LiquidityFlagValue('Z')
      , ROUTED_TO_NASDAQ_USING_INET_REMOVES_LIQUIDITYB= LiquidityFlagValue('2')
      , ADD_LIQUIDITY_PRE_AND_POST_MARKET_A_C         = LiquidityFlagValue('3')
      , ADD_LIQUIDITY_PRE_AND_POST_MARKET_B           = LiquidityFlagValue('4')
      , CUSTOMER_INTERNALIZATION_PRE_POST_MARKET      = LiquidityFlagValue('5')
      , REMOVES_LIQUIDITY_PRE_POST_MARKET_ALL_TAPES   = LiquidityFlagValue('6')
      , ROUTED_PRE_POST_MARKET                        = LiquidityFlagValue('7')
      , ROUTED_TO_AMEX_ADDS_LIQUIDITY                 = LiquidityFlagValue('8')
      , ROUTED_TO_ARCA_ADDS_LIQUIDITY_TAPES_A_C       = LiquidityFlagValue('9')
      , ROUTED_TO_ARCA_ADDS_LIQUIDITY_TAPE_B      =LiquidityFlagValue('1', '0')
      , MIDPOINT_MATCH_CROSS_SAME_MPID            =LiquidityFlagValue('A', 'A')
      , REMOVE_LIQUIDITY_FROM_OUR_BOOK_TAPE_B     =LiquidityFlagValue('B', 'B')
      , ROUTED_TO_BYX_USING_ROBY_OR_ROUC          =LiquidityFlagValue('B', 'Y')
      , LISTING_MARKET_CLOSE_EXCLUDING_ARCA       =LiquidityFlagValue('C', 'L')
      , LIQUIDITY_REMOVER_VIA_CLC_ELIGIBLE_RTSTRAT=LiquidityFlagValue('C', 'R')
      , NON_DISPLAYED_ADDS_LIQUIDITY_USING_MDO    =LiquidityFlagValue('D', 'M')
      , NON_DISPLAYED_REMOVES_LIQUIDITY_USING_MDO =LiquidityFlagValue('D', 'T')
      , MIDPOINT_MATCH_MAKER                      =LiquidityFlagValue('M', 'M')
      , MIDPOINT_MATCH_TAKER                      =LiquidityFlagValue('M', 'T')
      , DIRECT_EDGE_OPENING                       =LiquidityFlagValue('O', 'O')
      , MIDPOINT_ROUTING_RMPT_ADDS_LIQUIDITY      =LiquidityFlagValue('P', 'A')
      , REMOVES_LIQUIDITY_FROM_MIDPT_MATCH_EDGX   =LiquidityFlagValue('P', 'I')
      , LIQUIDITY_REMOVER_VIA_CLC_ONLY_RTSTRAT    =LiquidityFlagValue('P', 'R')
      , MIDPOINT_ROUTING_RMPT_REMOVES_LIQUIDITY   =LiquidityFlagValue('P', 'T')
      , MIDPOINT_ROUTING_RMPT_ROUTED_OUT          =LiquidityFlagValue('P', 'X')
      , ROUTED_TO_NASDAQ_BX_ADDS_LIQUIDITY        =LiquidityFlagValue('R', 'B')
      , ROUTED_TO_NSX_ADDS_LIQUIDITY              =LiquidityFlagValue('R', 'C')
      , ADDED_LIQUIDITY_USING_ROUTE_PEGORDER      =LiquidityFlagValue('R', 'P')
      , ROUTED_USING_ROUQ                         =LiquidityFlagValue('R', 'Q')
      , ROUTED_TO_EDGA_OR_EDGX_IOCT_OR_IOCX       =LiquidityFlagValue('R', 'R')
      , ROUTED_TO_NASDAQ_PSX_ADDS_LIQUIDITY       =LiquidityFlagValue('R', 'S')
      , ROUTED_USING_ROUT                         =LiquidityFlagValue('R', 'T')
      , ROUTED_TO_CBSX_ADDS_LIQUIDITY             =LiquidityFlagValue('R', 'W')
      , ROUTED_USING_ROUX                         =LiquidityFlagValue('R', 'X')
      , ROUTED_TO_BATS_BYX_ADDS_LIQUIDITY         =LiquidityFlagValue('R', 'Y')
      , ROUTED_TO_BATS_BZX_ADDS_LIQUIDITY         =LiquidityFlagValue('R', 'Z')
      , ROUTED_USING_SWPA_SWPB_ALL_EXCEPT_NYSE    =LiquidityFlagValue('S', 'W')
      , LIQUIDITY_REMOVER_VIA_NONCLC_RTSTRAT      =LiquidityFlagValue('X', 'R')
      , RETAIL_ORDER_ADDS_LIQUIDITY               =LiquidityFlagValue('Z', 'A')
      , RETAIL_ORDER_REMOVES_LIQUIDITY            =LiquidityFlagValue('Z', 'R')
    };

    struct LiquidityFlagStruct {
        LiquidityFlag16 first_two;
        std::uint8_t padding[3];
    } __attribute__((packed));

    union LiquidityFlag {
        LiquidityFlagRaw raw;
        LiquidityFlagStruct structured;
    } __attribute__((packed));
    I01_ASSERT_SIZE(LiquidityFlag, 5);

    typedef std::uint64_t MatchNumber;

    enum class RejectReason : std::uint8_t {
        NOT_SUPPORTED_IN_CURRENT_TRADING_SESSION          = 'A'
      , BULK_ORDER_NUMBER_ORDERS_EXCEEDED_THRESHOLD       = 'B'
      , EXCHANGE_CLOSED                                   = 'C'
      , INVALID_DISPLAY_TYPE                              = 'D'
      , EXCHANGE_OPTION                                   = 'E'
      , HALTED                                            = 'F'
      , CANNOT_EXECUTE_IN_CURRENT_TRADING_STATE           = 'H'
      , ORDER_NOT_FOUND                                   = 'I'
      , FIRM_NOT_AUTHORIZED_FOR_CLEARING                  = 'L'
      , OTHER                                             = 'O'
      , ORDER_ALREADY_IN_PENDING_CANCEL_OR_REPLACE_STATE  = 'P'
      , INVALID_QUANTITY                                  = 'Q'
      , RISK_CONTROL_REJECT                               = 'R'
      , INVALID_STOCK                                     = 'S'
      , TEST_MODE                                         = 'T'
      , ORDER_HAS_INVALID_OR_UNSUPPORTED_CHARACTERISTIC   = 'U'
      , ORDER_REJECTED_BECAUSE_MAX_ORDER_RATE_EXCEEDED    = 'V'
      , INVALID_PRICE                                     = 'X'
    };

    enum class InResponseTo : std::uint8_t {
        NEW_ORDER = 'O'
      , CANCEL    = 'X'
      , REPLACE   = 'U'
    };

    enum class CanceledReason : std::uint8_t {
        USER_REQUESTED_CANCEL           = 'U'
      , IMMEDIATE_OR_CANCEL             = 'I'
      , TIMEOUT                         = 'T'
      , MANUALLY_BY_DIRECT_EDGE         = 'S'
      , REGULATORY_RESTRICTION          = 'D'
      , WILL_RESULT_IN_LOCKED_OR_CROSSED= 'A'
      , ANTI_INTERNALIZATION            = 'B'
    };

    enum class EventCode : std::uint8_t {
        START_OF_DAY = 'S'
      , END_OF_DAY   = 'E'
    };

    enum class BrokenTradeReason : std::uint8_t {
        MANUALLY_BROKEN_BY_DIRECTEDGE = 'S'
    };

    enum class PriceCorrectionReason : std::uint8_t {
        MANUALLY_PRICE_CORRECTED_BY_DIRECTEDGE = 'S'
    };

    enum class AIMethod : std::uint8_t {
        CANCEL_NEWEST                                                 = 'A'
      , CANCEL_OLDEST                                                 = 'B'
      , CANCEL_BOTH                                                   = 'C'
      , CANCEL_SMALLEST                                               = 'D'
      , DECREMENT_LARGER_CANCEL_SMALLER_ORDER_QTY_AND_LEAVES_REDUCED  = 'E'
      , DECREMENT_LARGER_CANCEL_SMALLER_LEAVES_REDUCED                = 'F'
    };

    enum class AIIdentifier : std::uint8_t {
        MPID                      = 'A'
      , MEMBER_ID                 = 'B'
      , MPID_AND_AI_GROUP_ID      = 'C'
    };

    typedef std::uint16_t AIGroupID;

    enum class CanceledOrderState : std::uint8_t {
        INBOUND = 'I'
      , POSTED  = 'P'
    };

    typedef std::uint8_t MemberID[6];
    typedef std::uint8_t ContraTokenOrClOrderID[20];

} } } } }

