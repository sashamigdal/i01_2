#pragma once

#include <cstdint>

#include <map>
#include <tuple>
#include <type_traits>

#include <boost/pool/pool_alloc.hpp>

#include <i01_core/Lock.hpp>
#include <i01_core/macro.hpp>
#include <i01_core/MIC.hpp>
#include <i01_core/Time.hpp>

#include <i01_md/BookBase.hpp>

namespace i01 { namespace MD {

class L2PriceLevel {
public:
    /// const if we don't own the orders.
    using Price = MD::Price;
    using Size = MD::Size;
    using NumOrders = MD::NumOrders;
    typedef core::Timestamp Timestamp;
    const static int PriceIndex = 0;
    const static int SizeIndex = 1;
    const static int NumOrdersIndex = 2;

    L2PriceLevel(const Price & p, const Size &s, const NumOrders &no, const Timestamp &ts) :
        m_price(p), m_quantity(s), m_num_orders(no), m_last_updated(ts), m_created(ts) {}

    const Price& price() const { return m_price; }
    const Size& size() const { return m_quantity; }
    void size(const Size &sz) { m_quantity = sz; }
    const NumOrders& num_orders() const { return m_num_orders; }
    void num_orders(const NumOrders &no) { m_num_orders = no; }
    const Timestamp& last_updated() const { return m_last_updated; }
    void last_updated(const Timestamp &ts) { m_last_updated = ts; }
    const Timestamp& created() const { return m_created; }

private:
    Price m_price;
    Size m_quantity;
    NumOrders m_num_orders;
    Timestamp m_last_updated;
    Timestamp m_created;
};

/// \internal Use Compare = std::greater for bids, std::less for asks.
template <template <typename Price> class Compare, Price EMPTY_PRICE, typename RWMutexT = core::SpinRWMutex>
class L2BookHalf {
public:
    typedef L2PriceLevel PriceLevel;
    typedef PriceLevel::Price Price;
    typedef PriceLevel::Size Size;
    typedef PriceLevel::NumOrders NumOrders;
    typedef core::Timestamp Timestamp;

private:
    typedef Compare<Price> Comparator;
    typedef RWMutexT RWMutex;
    /// A single L2 book price level.

    typedef std::map< Price
                      , PriceLevel
                      , Compare<Price>
                      , boost::fast_pool_allocator<std::pair<Price, PriceLevel>>> BookHalfStorage;

public:
    using const_iterator = typename BookHalfStorage::const_iterator;

public:
    L2BookHalf() = default;
    virtual ~L2BookHalf() = default;

    Summary best() const {
        return best_quote();
    }

    L2Quote best_quote() const {
        const auto & ret = m_bookhalf_storage.begin();
        if (ret != m_bookhalf_storage.end()) {
            return L2Quote{ ret->first, ret->second.size(), ret->second.num_orders() };
        } else {
            /* empty book */
            return L2Quote{ EMPTY_PRICE, 0, 0 };
        }
    }

    /// Replace a price level... could delete if size is 0 ... return true if top of book is updated
    bool replace(const Price &p, const Size &s, const NumOrders &n, const Timestamp &ts) {
        m_level_deleted = s == 0;
        m_reduced_size = m_level_deleted;

        auto it = m_bookhalf_storage.find(p);

        if (it == m_bookhalf_storage.end()) {
            // this price level does not exist...
            if (m_level_deleted) {
                // TODO: add logging ... trying to remove a nonexistent price level
                m_deleted_level_existed = false;
                return false;
            }
            m_level_added = true;

            it = m_bookhalf_storage.emplace(std::make_pair(Price(p), PriceLevel(p, s, n, ts))).first;
        } else {
            m_level_added = false;
            if (m_level_deleted) {
                // removing an existing price level
                const bool ret = it == m_bookhalf_storage.begin();
                m_bookhalf_storage.erase(it);
                m_deleted_level_existed = true;
                return ret;
            } else {
                // udpating existing price level ... we just replace the order
                m_reduced_size = s < it->second.size();
                it->second.size(s);
                it->second.num_orders(n);
                it->second.last_updated(ts);
            }
        }

        // check for crosses here?

        if (it == m_bookhalf_storage.begin()) {
            return true;
        } else {
            return false;
        }
    }

    /// Clear the book, e.g. if stale due to dropped messages.
    void clear() {
        m_bookhalf_storage.clear();
    }

    /// Did the last order add a price level?
    bool last_order_added_level() const { return m_level_added; }
    /// Did the last order delete a price level?
    bool last_order_deleted_level() const { return m_level_deleted; }
    /// Did the last order delete an exitsing level?
    bool last_order_deleted_existing_level() const { return m_deleted_level_existed; }
    /// Did the last replace reduce the size at the price level?
    bool last_update_reduced_size() const { return m_reduced_size; }

    /// Const iterator to the price levels
    const_iterator find(const Price p) const { return m_bookhalf_storage.find(p); }
    /// The end market iterator for price levels
    const_iterator end() const { return m_bookhalf_storage.end(); }

private:
    BookHalfStorage m_bookhalf_storage;
    bool m_level_added;
    bool m_level_deleted;
    bool m_deleted_level_existed;
    bool m_reduced_size;
};

template <typename Mutex = core::SpinRWMutex>
using BidL2BookHalf = L2BookHalf<std::greater, BookBase::EMPTY_BID_PRICE, Mutex>;

template <typename Mutex = core::SpinRWMutex>
using AskL2BookHalf = L2BookHalf<std::less, BookBase::EMPTY_ASK_PRICE, Mutex>;

class L2Book : public BookBase {
public:
    typedef BidL2BookHalf<> bidhalf_type;
    typedef AskL2BookHalf<> askhalf_type;

    typedef L2PriceLevel::Timestamp Timestamp;

    using Mutex = core::SpinRWMutex;

public:
    L2Book() = delete;
    L2Book(const core::MIC & mkt) : BookBase(mkt) {}
    L2Book(const core::MIC & mkt, const SymbolIndex &idx) : BookBase(mkt, idx) {}
    virtual ~L2Book() = default;

    bool replace(bool is_bid, const Price &p, const Size &s, const NumOrders &n, const Timestamp &ts);

    virtual Summary best_bid() const override final { Mutex::scoped_lock lock(m_mutex, /* write= */ false); return m_bids.best(); }
    virtual Summary best_ask() const override final { Mutex::scoped_lock lock(m_mutex, /* write= */ false); return m_asks.best(); }

    virtual L2Quote l2_bid(const Price& p) const override final { Mutex::scoped_lock lock(m_mutex, /* write= */ false); return unsafe_l2_bid(p); }
    virtual L2Quote l2_ask(const Price& p) const override final { Mutex::scoped_lock lock(m_mutex, /* write= */ false); return unsafe_l2_ask(p); }

    FullSummary best() const override final {
        Mutex::scoped_lock lock(m_mutex, /* write= */ false);
        return { unsafe_best_bid(), unsafe_best_ask() };
    }

    bool crossed() const;
    bool locked() const;

    bool last_order_added_level(bool bid) const;
    bool last_order_deleted_level(bool bid) const;
    bool last_order_deleted_existing_level(bool bid) const;
    bool last_update_reduced_size(bool bid) const;

    virtual void clear() override final;

protected:
    L2Quote unsafe_best_bid() const;
    L2Quote unsafe_best_ask() const;

    L2Quote unsafe_l2_bid(const Price&) const;
    L2Quote unsafe_l2_ask(const Price&) const;

private:
    bidhalf_type m_bids;
    askhalf_type m_asks;

    mutable Mutex m_mutex;
};


}}
