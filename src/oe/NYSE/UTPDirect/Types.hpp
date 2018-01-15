#pragma once

#include <cstdint>
#include <array>

namespace i01 { namespace OE { namespace NYSE { namespace UTPDirect { namespace Types {

    enum class MessageType : std::uint16_t {
        HEARTBEAT_MESSAGE                   = 0x0001 /* 0 1 */
      , TEST_MESSAGE                        = 0x0011 /* 1 1 */
      , LOGON                               = 0x0021 /* A 1 */
      , NEW_ORDER_1                         = 0x0041 /* D 1 */
      , NEW_ORDER_2                         = 0x0042 /* D 2 */
      , NEW_ORDER_3                         = 0x0043 /* D 3 */
      , NEW_ORDER_4                         = 0x0044 /* D 4 */
      , CANCEL_ORDER                        = 0x0061 /* F 1 */
      , CANCEL_REPLACE_1                    = 0x0071 /* G 1 */
      , CANCEL_REPLACE_2                    = 0x0072 /* G 2 */
      , CANCEL_REPLACE_3                    = 0x0073 /* G 3 */
      , CANCEL_REPLACE_4                    = 0x0074 /* G 4 */
      , FILLED                              = 0x0081 /* 2 1 */
      , ACK                                 = 0x0091 /* a 1 */
      , CANCEL_REQUEST_ACK                  = 0x00A1 /* 6 1 */
      , CANCEL_REPLACE_ACK                  = 0x00B1 /* E 1 */
      , FILLED_VERBOSE                      = 0x00C1 /* X 1 */
      , UROUT_CANCEL_CONFIRMATION           = 0x00D1 /* 4 1 */
      , REPLACED                            = 0x00E1 /* 5 1 */
      , REJECT                              = 0x00F1 /* 8 1 */
      , BUST_OR_CORRECT                     = 0x0101 /* C 1 */
      , LOGON_REJECT                        = 0x0141 /* L 1 */
    };

    typedef std::uint16_t MsgLength;

    union MsgSeqNum {
        std::uint8_t raw[4];
        std::uint32_t integral;
        enum class Special : std::int32_t {
            REPLAY_ALL              =  0
          , CONTINUE_WITHOUT_REPLAY = -1
        } special;
    } __attribute__((packed));

    typedef std::uint8_t LoginID[12];
    typedef std::uint8_t CompID[5];

    typedef std::uint8_t MessageVersionProfile[32];

    enum class CancelOnDisconnect : std::uint8_t {
        NO_CANCEL_ON_DISCONNECT                  = '0'
      , CANCEL_ALL_EXCEPT_GTC_MOC_LOC_MOO_LOO_CO = '1'
    };

    enum class LogonRejectType : std::uint16_t {
        SUCCESS                           = 0x0000
      , SYSTEM_UNAVAILABLE                = 0x0001
      , INVALID_SEQNUM                    = 0x0002
      , CLIENT_SESSION_ALREADY_EXISTS     = 0x0003
      , CLIENT_SESSION_DISABLED           = 0x0004
      , CONNECTION_TYPE                   = 0x0005
    };

    typedef std::uint32_t Quantity;
    typedef std::uint32_t Price;
    enum class PriceScale : std::uint8_t {
        ZERO  = '0'
      , ONE   = '1'
      , TWO   = '2'
      , THREE = '3'
      , FOUR  = '4'
    };

    typedef std::uint8_t Symbol[11];
    
    enum class ExecInst : std::uint8_t {
        DO_NOT_INCREASE = 'E'
      , DO_NOT_REDUCE   = 'F'
    };

    enum class Side : std::uint8_t {
        BUY           = '1'
      , SELL          = '2'
      , BUY_MINUS     = '3'
      , SELL_PLUS     = '4'
      , SHORT         = '5'
      , SHORT_EXEMPT  = '6'
    };

    enum class OrderType : std::uint8_t {
        MARKET              = '1'
      , LIMIT               = '2' /* + NASDAQ securities */
      , STOP                = '3'
      , MOC                 = '5'
      , LOC                 = 'B'
    };

    enum class TimeInForce : std::uint8_t {
        DAY = '0'
      , GTC = '1'
      , OPG = '2'
      , IOC = '3'
    };

    enum class Capacity : std::uint8_t {
        // TODO: fill in A-Z from appendix C.
    };

    enum class RoutingInstruction : std::uint8_t {
        NX    = '7'
      , DNS   = 'D'
      , SOC   = 'S'
      , ISO   = 'I'
      , CO    = 'C'
    };

    enum class DOTReserve : std::uint8_t {
        YES = 'Y'
      , NO  = 'N' /* default */
    };

    typedef std::uint8_t SubID[5];
    typedef std::uint8_t ClearingFirm[5];
    typedef std::uint8_t Account[10];
    
    /// Format: BBB NNNN/MMDDYYYY.  See spec.
    typedef std::uint8_t ClientOrderID[17];

    // TODO:  finish...

} } } } }
