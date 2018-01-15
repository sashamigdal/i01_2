#pragma once

#include <i01_core/Lock.hpp>

#include <i01_md/BookBase.hpp>
#include <i01_md/BookMuxListener.hpp>
#include <i01_md/DecoderEvents.hpp>
#include <i01_md/LastSale.hpp>
#include <i01_md/NASDAQ/MoldUDP64/Types.hpp>
#include <i01_md/RawListener.hpp>
#include <i01_md/Symbol.hpp>
#include <i01_md/SymbolState.hpp>

namespace i01 { namespace MD {

// this is some meta-programming magic to iterate over the listeners
// tuple and call the passed method function pointer with given
// arguments.  We need to decay the arguments to the function pointer
// because otherwise we can't get a template match, since the function
// pointer signature has to match the passed arguments exactly.

template<int I, int TSize, typename Tuple, typename MemberFnPtr, typename... Args>
struct CallListenersImpl : public CallListenersImpl<I + 1, TSize, Tuple, MemberFnPtr, Args...>
{
    void operator() (Tuple& t, MemberFnPtr mfp, Args&&... args) {
        (std::get<I>(t)->*mfp)(std::forward<Args>(args)...);
        CallListenersImpl<I + 1, TSize, Tuple, MemberFnPtr, Args...>::operator() (t, mfp, std::forward<Args>(args)...);
    }
};

template<int I, typename Tuple, typename MemberFnPtr, typename... Args>
struct CallListenersImpl<I, I, Tuple, MemberFnPtr, Args...>
{
    void operator() (Tuple&, MemberFnPtr, Args&&...) {}
};

template<typename... ListenerTypes, typename MemberFnPtr, typename... Args>
static void call_listeners(std::tuple<ListenerTypes...>& t, MemberFnPtr mfp, Args&&...args)
{
    CallListenersImpl<0, sizeof...(ListenerTypes), std::tuple<ListenerTypes...>, MemberFnPtr, Args&&...>() (t, mfp, std::forward<Args>(args)...);
}

class BookMuxBase {
public:
    using MIC = core::MIC;
    using SymbolMutex = core::SpinRWMutex;
    using UnitIndex = std::uint8_t;

public:
    static const int NUM_UNIT_INDEX = std::numeric_limits<UnitIndex>::max() + 1;

public:
    BookMuxBase(const core::MIC & m);
    virtual ~BookMuxBase();

    const MIC & mic() const { return m_mic; }

    const BookBase * operator[](const EphemeralSymbolIndex esi) noexcept { return m_bookinfo_by_esi[esi].book_p; }

    const SymbolState& symbol_state(const EphemeralSymbolIndex esi) const noexcept { return m_bookinfo_by_esi[esi].state; }

    const LastSale& last_sale(const EphemeralSymbolIndex esi) const noexcept { return m_bookinfo_by_esi[esi].last_sale; }

    int clear_books_in_unit(const UnitIndex unit) noexcept;

    bool clear_book(const EphemeralSymbolIndex esi) noexcept;

protected:
    MIC m_mic;
    // TODO FIXME: Move SymbolState and BookBase* into single array for cache
    // coherency, and so multiple lookups by esi aren't necessary.

    virtual BookBase * create_book_for_symbol(const EphemeralSymbolIndex&) = 0;

    /// this is not thread safe
    BookBase *& book_from_esi_(const EphemeralSymbolIndex esi) noexcept { return m_bookinfo_by_esi[esi].book_p; }
    SymbolState& state_from_esi_(const EphemeralSymbolIndex esi) noexcept { return m_bookinfo_by_esi[esi].state; }
    LastSale & last_sale_from_esi_(const EphemeralSymbolIndex esi) noexcept { return m_bookinfo_by_esi[esi].last_sale; }
    SymbolMutex& symbol_mutex_from_esi_(const EphemeralSymbolIndex esi) noexcept { return m_bookinfo_by_esi[esi].mutex; }
private:
    struct BookInfo {
        BookBase *book_p;
        SymbolState state;
        LastSale last_sale;
        SymbolMutex mutex;
    };

private:
    std::array<BookInfo, (int)MD::NUM_SYMBOL_INDEX> m_bookinfo_by_esi;
    std::array<std::vector<EphemeralSymbolIndex>, NUM_UNIT_INDEX> m_esi_by_unit;
};

template<typename BookType, typename... Listeners>
class MLBookMux : public BookMuxBase, public i01::MD::RawListener<MLBookMux<BookType> > {
public:
    static_assert(std::is_base_of<MD::BookBase, BookType>::value, "MLBookMux: BookType must be derived from MD::BookBase");
    using Listener = BookMuxListener;
    using Book = BookType;
    using MIC = core::MIC;
    using Timestamp = core::Timestamp;
    using BaseMux = MLBookMux<BookType, Listeners...>;
    using ListenersTuple = std::tuple<Listeners*...>;
    // TODO: we should verify that every type in Listeners... is derived from BookMuxListener

protected:
    using BookMuxBase::m_mic;

public:
    MLBookMux(const core::MIC & m, Listeners*... listeners);
    virtual ~MLBookMux() = default;

    virtual BookType * create_book_for_symbol(const EphemeralSymbolIndex&) override ;

    // we do this std::decay because otherwise the arguments we pass
    // to this must be the exact match types as the arguments to the
    // member func .. also, we are wrapping this in a call here so
    // that we can add support for multiple listeners at a later dat
    template<typename MemberFnPtr, typename...Args>
    void notify(MemberFnPtr mfp, Args&&...args) {
        call_listeners(m_listeners, mfp, std::forward<Args>(args)...);
    }

    template<typename MemberFnPtr>
    void notify(MemberFnPtr mfp, TradeEvent&& te) {
        // update last sale
        if (!te.m_cross) {
            this->last_sale_from_esi_(te.m_book.symbol_index()) = LastSale{te.m_price, te.m_timestamp};
        }

        call_listeners(m_listeners, mfp, std::forward<TradeEvent>(te));
    }

    template<typename MemberFnPtr>
    void notify(MemberFnPtr mfp, L3ExecutionEvent&& ee) {
        // update last sale
        if (!ee.m_nonprintable) {
            this->last_sale_from_esi_(ee.m_book.symbol_index()) = LastSale{ee.m_exec_price, ee.m_timestamp};
        }

        call_listeners(m_listeners, mfp, std::forward<L3ExecutionEvent>(ee));
    }

    // ignore heartbeats from the decoder
    template<typename...Args>
    void on_raw_msg(const Timestamp &ts, const HeartbeatMsg &, ...) {}

    // UnhandledMsgs from the decoder remain unhandled.  There are
    // many arguments here b/c this message is shared across
    // exchanges, so needs to be generic enough for all.
    void on_raw_msg(const Timestamp &ts, const UnhandledMsg &, std::uint32_t addr, std::uint16_t port, std::uint8_t unit, std::uint64_t seqnum, std::uint8_t msg_type, void *msg) {}

    // There are
    // many arguments here b/c this message is shared across
    // exchanges, so needs to be generic enough for all.
    void on_raw_msg(const Timestamp &ts, const GapMsg &, std::uint32_t addr, std::uint16_t port, std::uint8_t unit, std::uint64_t expected, std::uint64_t received, const Timestamp &last_ts) {
        notify(&Listener::on_gap, GapEvent{ts, m_mic, expected, received, last_ts});
    }

    void on_raw_msg(const Timestamp &ts, const GapMsg &, const NASDAQ::MoldUDP64::Types::Session &session, std::uint64_t expected, std::uint64_t received, const Timestamp &last_ts) {
        notify(&Listener::on_gap, GapEvent{ts, m_mic, expected, received, last_ts});
    }

    void on_raw_msg(const Timestamp &ts, const StartOfPktMsg &, std::uint64_t seqnum, std::uint32_t index) {
        notify(&Listener::on_start_of_data, PacketEvent{ts, m_mic});
    }

    void on_raw_msg(const Timestamp &ts, const EndOfPktMsg &, std::uint64_t seqnum, std::uint32_t index) {
        notify(&Listener::on_end_of_data, PacketEvent{ts, m_mic});
    }

    void on_raw_msg(const Timestamp& ts, const TimeoutMsg&, bool started, const std::string& name, int unit_index, const Timestamp& last_ts) {
        notify(&Listener::on_timeout_event, TimeoutEvent{ts, m_mic, last_ts, unit_index, name, started ? TimeoutEvent::EventCode::TIMEOUT_START : TimeoutEvent::EventCode::TIMEOUT_END});
    }


protected:
    ListenersTuple m_listeners;
};

template<typename BT, typename...LT>
MLBookMux<BT,LT...>::MLBookMux(const core::MIC & m, LT*... nl) :
    BookMuxBase::BookMuxBase(m), m_listeners(std::make_tuple(nl...))
{
}

template<typename BT, typename...LT>
BT * MLBookMux<BT,LT...>::create_book_for_symbol(const EphemeralSymbolIndex& esi)
{
    if (book_from_esi_(esi) == nullptr) {
        book_from_esi_(esi) = new BT(m_mic, esi);
    }

    return static_cast<BT *>(book_from_esi_(esi));
}

}}
