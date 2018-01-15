#pragma once

#include <cstdint>
#include <iosfwd>
#include <string.h>

#include <i01_core/macro.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/Version.hpp>
#include <i01_core/MIC.hpp>
#include <i01_md/Symbol.hpp>
#include <i01_oe/Types.hpp>
#include <i01_oe/Order.hpp>

namespace i01 { namespace OE {
namespace OrderLogFormat {
    // File format version corresponds to a namespace.  When you change the
    // format in a backwards-incompatible way, uninline this
    // namespace and create a new v${n+1} inline namespace.
    inline namespace v1 {

    static const std::uint32_t MAGIC_NUMBER = 0x4F4C4649;
    static_assert(MAGIC_NUMBER != __bswap_constant_32(MAGIC_NUMBER), "OrderLogFormat magic number is used for endianness detection, so it must not be symmetric.");

    typedef MD::EphemeralSymbolIndex ESI;
union SessionName {
    std::uint64_t               u64;
    std::array<std::uint8_t, sizeof(std::uint64_t)>     arr;
    std::string to_string() const;
};

using EngineName = SessionName;

    inline SessionName string_to_session_name(const std::string& s)
    {
        SessionName n{0};
        ::strncpy((char *)&(n.u64)
                , s.c_str()
                , sizeof(n.u64));
        return n;
    }

    /// This header struct should not change between library (namespace) versions,
    //  only the contents.
    struct FileHeader {
        std::uint32_t magic_number;
        std::uint64_t version;
        /// Git commit hash of engine code used to write this log file.
        std::uint8_t git_commit_sha1[40];

        bool reset()
        {
            magic_number = MAGIC_NUMBER;
            // on big endian systems: magic_number = 0x41464C4F;
            version = g_INTEGER_VERSION;

            size_t n = ::strnlen(g_COMMIT, sizeof(git_commit_sha1) + 1);
            if (n == sizeof(git_commit_sha1)) {
                ::memset(git_commit_sha1, 0, sizeof(git_commit_sha1));
                ::memcpy(git_commit_sha1, g_COMMIT, sizeof(git_commit_sha1));
            }
            return true;
        }

    } __attribute__((packed));

    /// A new FileHeader is required after this message.
    struct FileTrailer {
        std::uint32_t terminator;
    } __attribute__((packed));

    enum class MessageType : std::uint8_t {
        UNKNOWN               = 0,
        TIMESTAMP             = 1,    //< TimestampBody
        START_OF_LOG          = 2,    //< FileTrailer
        END_OF_LOG            = 3,    //< FileTrailer
        // ...
        NEW_INSTRUMENT        = 4,    //< NewInstrumentBody
        NEW_ORDER             = 5,    //< (UNUSED) NewOrderBody (UNUSED)
        ORDER_LOCAL_REJECT    = 6,    //< OrderLocalRejectBody
        ORDER_SENT            = 7,    //< OrderSentBody
        ORDER_ACKNOWLEDGEMENT = 8,    //< OrderAckBody
        ORDER_PARTIAL_FILL    = 9,    //< OrderFillBody
        ORDER_FILL            = 10,    //< OrderFillBody
        ORDER_CANCEL_REQUEST  = 11,   //< OrderCxlReqBody
        ORDER_PARTIAL_CANCEL  = 12,   //< OrderCxlBody
        ORDER_CANCEL          = 13,   //< OrderCxlBody
        ORDER_CXLREPL_REQUEST = 14,   //< OrderCxlReplaceReqBody
        ORDER_CANCEL_REPLACE  = 15,   //< OrderCxlReplaceBody
        ORDER_REMOTE_REJECT   = 16,   //< OrderRejectBody
        ORDER_CANCEL_REJECT   = 17,   //< OrderCxlRejectBody
        ORDER_DESTROY         = 18,   //< OrderDestroyBody
        // ...
        POSITION              = 19,   //< PositionBody
        MANUAL_POSITION_ADJ   = 20,   //< ManualPositionAdjBody
        // ...
        NEW_SESSION           = 21,   //< SessionBody
        ORDER_SESSION_DATA    = 22,   //< OrderSessionDataHeader followed by binary data.
        // ...
        NEW_ACCOUNT           = 23,   //< Strategy/LocalAccount definition.
        MAX_MESSAGE_TYPE
    };

    struct MessageHeader {
        std::uint16_t length;
        std::uint8_t type;
        std::uint8_t subtype;
        core::POSIXTimeSpec timestamp;
    } __attribute__((packed));

    template <typename T>
    struct Message {
        Message(MessageType type_, Timestamp ts)
            : hdr{sizeof(body), (std::uint8_t)type_, 0, ts} {}
        Message(MessageType type_, Timestamp ts, T&& body_)
            : hdr{sizeof(body), (std::uint8_t)type_, 0, ts}
            , body(std::forward<T>(body_)) {}
        MessageHeader hdr;
        T body;
    } __attribute__((packed));

    struct TimestampBody {
        core::POSIXTimeSpec ts;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const TimestampBody& body);

    struct NewInstrumentBody {
        ESI esi;
        core::MICEnum listing_market;
        std::uint8_t symbol[10];
        std::uint8_t bloomberg_gid_figi[12];
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const NewInstrumentBody& body);

    struct PositionBody {
        EngineID                            source;
        NewInstrumentBody                   instrument;
        Quantity                            qty;
        Price                               avg_price;
        Price                               mark_price;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const PositionBody& body);

    struct ManualPositionAdjBody {
        ESI esi;
        std::int32_t delta_trading_qty;
        std::int32_t delta_start_qty;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const ManualPositionAdjBody& body);

    struct NewSessionBody {
        SessionName name;
        core::MICEnum market_mic;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const NewSessionBody& body);

struct OrderIdentifier {
    LocalAccount local_account;
    LocalID local_id;
    SessionName session_name;
    bool operator==(const OrderIdentifier&a) const;
} __attribute__((packed));
std::ostream& operator<<(std::ostream&, const OrderIdentifier&);

    struct NewOrderBody {
        ESI instrument_esi;
        core::MICEnum market_mic; // e.g. XNYS, XNAS, ARCX, ...
        OrderIdentifier oid;
        ClientOrderID client_order_id;
        Price orig_price;
        Size size;
        Side side;
        TimeInForce tif;
        OrderType type;
        UserData user_data;
            // NOTE: Do *NOT* recover user_data into the new Order object,
            //       just log it for debugging purposes.
    } __attribute__((packed));
std::ostream& operator<<(std::ostream&, const NewOrderBody&);

    struct OrderSessionDataHeader {
        OrderIdentifier oid;
    } __attribute__((packed));
    std::ostream& operator<<(std::ostream& os, const OrderSessionDataHeader& body);

    struct OrderLocalRejectBody {
        NewOrderBody order;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const OrderLocalRejectBody& body);

    struct OrderSentBody {
        NewOrderBody order;

        struct compare {
            bool operator()(const OrderSentBody&, const OrderSentBody&) const;
        };
    } __attribute__((packed));
std::ostream& operator<<(std::ostream&, const OrderSentBody&);

    struct OrderAckBody {
        OrderIdentifier oid;
        Size open_size;
        ExchangeTimestamp exchange_ts;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream&, const OrderAckBody&);

    struct OrderFillBody {
        OrderIdentifier oid;
        Size filled_qty;
        Price filled_price;
        FillFeeCode filled_fee_code;
        ExchangeTimestamp exchange_ts;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const OrderFillBody& body);

    struct OrderCxlReqBody {
        OrderIdentifier oid;
        Size new_qty;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const OrderCxlReqBody& body);

    struct OrderCxlBody {
        OrderIdentifier oid;
        Size cxled_qty;
        ExchangeTimestamp exchange_ts;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const OrderCxlBody& body);

    struct OrderCxlReplaceReqBody {
        OrderIdentifier old_oid;
        NewOrderBody new_order;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const OrderCxlReplaceReqBody& body);

    struct OrderCxlReplaceBody {
        OrderIdentifier old_oid;
        NewOrderBody new_order;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const OrderCxlReplaceBody& body);

    struct OrderRejectBody {
        OrderIdentifier oid;
        ExchangeTimestamp exchange_ts;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const OrderRejectBody& body);

    struct OrderCxlRejectBody {
        OrderIdentifier oid;
        ExchangeTimestamp exchange_ts;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const OrderCxlRejectBody& body);

    struct OrderDestroyBody {
        OrderIdentifier oid;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const OrderDestroyBody& body);

    struct NewAccountBody {
        LocalAccount la;
        std::uint8_t name[256];
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const NewAccountBody& body);

    } // namespace v1
} // namespace OrderLogFormat
} }
